use std::collections::HashSet;
use std::fs;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

use base64::{engine::general_purpose::STANDARD, Engine as _};
use tauri::ipc::Response;

use crate::error::{ApiError, ApiResult};
use crate::models::ImportOutcome;

/// 只读文件头判断格式，避免导入时整图解码。
fn sniff_image(path: &Path) -> Result<(), String> {
    let mut file = fs::File::open(path).map_err(|err| format!("无法打开文件：{err}"))?;
    let mut head = [0u8; 32];
    let read = file.read(&mut head).map_err(|err| format!("无法读取文件：{err}"))?;
    let head = &head[..read];

    let is_png = head.starts_with(&[0x89, b'P', b'N', b'G']);
    let is_jpeg = head.starts_with(&[0xFF, 0xD8, 0xFF]);
    let is_bmp = head.starts_with(b"BM");
    let is_webp = head.len() >= 12 && &head[0..4] == b"RIFF" && &head[8..12] == b"WEBP";

    if is_png || is_jpeg || is_bmp || is_webp {
        Ok(())
    } else {
        Err("不是可识别的图片格式（支持 PNG / JPEG / BMP / WebP）".to_string())
    }
}

#[tauri::command]
pub fn import_images(paths: Vec<String>) -> Vec<ImportOutcome> {
    let mut seen = HashSet::new();
    let mut outcomes = Vec::with_capacity(paths.len());

    for raw in paths {
        if !seen.insert(raw.clone()) {
            continue;
        }

        let path = PathBuf::from(&raw);
        let name = path
            .file_name()
            .map(|value| value.to_string_lossy().to_string())
            .unwrap_or_else(|| raw.clone());

        let result = if !path.is_file() {
            Err("文件不存在或不是普通文件".to_string())
        } else {
            sniff_image(&path)
        };

        outcomes.push(match result {
            Ok(()) => ImportOutcome { path: raw, name, ok: true, error: None },
            Err(error) => ImportOutcome { path: raw, name, ok: false, error: Some(error) },
        });
    }

    outcomes
}

#[tauri::command]
pub fn read_image(path: String) -> ApiResult<Response> {
    let bytes = fs::read(&path).map_err(|err| {
        ApiError::new("E_READ", format!("读取图片失败：{err}")).with_context(path.clone())
    })?;
    Ok(Response::new(bytes))
}

#[tauri::command]
pub fn ensure_dir(path: String) -> ApiResult<()> {
    fs::create_dir_all(&path).map_err(|err| {
        ApiError::new("E_DIR", format!("无法创建目录：{err}")).with_context(path.clone())
    })
}

/// PNG 以 base64 传入：图片字节不经过逐数字的 JSON 数组，避免大图传输时膨胀。
#[tauri::command]
pub fn write_png(path: String, png_base64: String) -> ApiResult<()> {
    let bytes = STANDARD
        .decode(png_base64.as_bytes())
        .map_err(|err| ApiError::new("E_WRITE", format!("图片数据解码失败：{err}")))?;
    atomic_write(Path::new(&path), &bytes)
}

/// 先写同目录临时文件，`fsync` 后 `rename`；失败时清理临时文件。
pub(crate) fn atomic_write(target: &Path, bytes: &[u8]) -> ApiResult<()> {
    let dir = target
        .parent()
        .filter(|value| !value.as_os_str().is_empty())
        .ok_or_else(|| ApiError::new("E_WRITE", "目标路径缺少目录"))?;

    if !dir.is_dir() {
        return Err(ApiError::new("E_DIR", "输出目录不存在")
            .with_context(dir.display().to_string()));
    }

    let file_name = target
        .file_name()
        .ok_or_else(|| ApiError::new("E_WRITE", "目标路径缺少文件名"))?
        .to_string_lossy()
        .to_string();

    let stamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|value| value.as_nanos())
        .unwrap_or(0);
    let temp = dir.join(format!(".{file_name}.tmp-{}-{stamp}", std::process::id()));

    let write_result = (|| -> std::io::Result<()> {
        let mut file = fs::File::create(&temp)?;
        file.write_all(bytes)?;
        file.sync_all()?;
        Ok(())
    })();

    if let Err(err) = write_result {
        let _ = fs::remove_file(&temp);
        return Err(ApiError::new("E_WRITE", format!("写入失败：{err}"))
            .with_context(target.display().to_string()));
    }

    if let Err(err) = fs::rename(&temp, target) {
        let _ = fs::remove_file(&temp);
        return Err(ApiError::new("E_WRITE", format!("重命名失败：{err}"))
            .with_context(target.display().to_string()));
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sniffs_supported_headers() {
        let dir = tempfile::tempdir().unwrap();
        let png = dir.path().join("a.png");
        fs::write(&png, [0x89, b'P', b'N', b'G', 0x0D]).unwrap();
        assert!(sniff_image(&png).is_ok());

        let text = dir.path().join("fake.png");
        fs::write(&text, b"hello world").unwrap();
        assert!(sniff_image(&text).is_err());
    }

    #[test]
    fn import_dedupes_and_reports_failures() {
        let dir = tempfile::tempdir().unwrap();
        let image = dir.path().join("shot.png");
        fs::write(&image, [0x89, b'P', b'N', b'G']).unwrap();
        let missing = dir.path().join("missing.png");

        let outcomes = import_images(vec![
            image.display().to_string(),
            image.display().to_string(),
            missing.display().to_string(),
        ]);

        assert_eq!(outcomes.len(), 2);
        assert!(outcomes[0].ok);
        assert!(!outcomes[1].ok);
    }

    #[test]
    fn atomic_write_creates_file_and_leaves_no_temp() {
        let dir = tempfile::tempdir().unwrap();
        let target = dir.path().join("out.png");
        atomic_write(&target, b"data").unwrap();

        assert_eq!(fs::read(&target).unwrap(), b"data");
        let leftovers: Vec<_> = fs::read_dir(dir.path())
            .unwrap()
            .filter_map(|entry| entry.ok())
            .filter(|entry| entry.file_name().to_string_lossy().starts_with('.'))
            .collect();
        assert!(leftovers.is_empty(), "不应残留临时文件");
    }

    #[test]
    fn atomic_write_rejects_missing_dir() {
        let dir = tempfile::tempdir().unwrap();
        let target = dir.path().join("nope").join("out.png");
        let error = atomic_write(&target, b"data").unwrap_err();
        assert_eq!(error.code, "E_DIR");
    }
}
