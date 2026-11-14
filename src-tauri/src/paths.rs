use std::path::PathBuf;

use tauri::{AppHandle, Manager};

use crate::error::{ApiError, ApiResult};

/// 应用数据目录下的数据库文件路径；目录不存在时创建。
pub fn db_path(app: &AppHandle) -> ApiResult<PathBuf> {
    let dir = app
        .path()
        .app_data_dir()
        .map_err(|err| ApiError::new("E_DB", format!("无法定位应用数据目录：{err}")))?;

    std::fs::create_dir_all(&dir).map_err(|err| {
        ApiError::new("E_DIR", format!("无法创建应用数据目录：{err}"))
            .with_context(dir.display().to_string())
    })?;

    Ok(dir.join("renderlaz.db"))
}
