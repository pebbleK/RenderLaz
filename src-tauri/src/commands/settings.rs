use std::collections::HashMap;

use rusqlite::Connection;
use tauri::State;

use crate::db::now_iso;
use crate::error::{ApiError, ApiResult};
use crate::models::AppSettings;
use crate::AppState;

fn read_all(conn: &Connection) -> ApiResult<HashMap<String, String>> {
    let mut stmt = conn.prepare("SELECT key, value FROM settings")?;
    let rows = stmt.query_map([], |row| Ok((row.get::<_, String>(0)?, row.get::<_, String>(1)?)))?;
    let mut map = HashMap::new();
    for row in rows {
        let (key, value) = row?;
        map.insert(key, value);
    }
    Ok(map)
}

fn from_map(map: &HashMap<String, String>) -> AppSettings {
    AppSettings {
        output_dir: map.get("output_dir").cloned().filter(|value| !value.is_empty()),
        blur_radius: map
            .get("blur_radius")
            .and_then(|value| value.parse::<f32>().ok())
            .filter(|value| (0.5..=64.0).contains(value))
            .unwrap_or(8.0),
        window_width: map.get("window_width").and_then(|value| value.parse::<i64>().ok()),
        window_height: map.get("window_height").and_then(|value| value.parse::<i64>().ok()),
        session_state: map.get("session_state").cloned().filter(|value| !value.is_empty()),
    }
}

/// 读设置永不失败：数据库不可用时回落到默认值（FR-023）。
#[tauri::command]
pub fn get_settings(state: State<'_, AppState>) -> AppSettings {
    match state.db.with(read_all) {
        Ok(map) => from_map(&map),
        Err(_) => AppSettings::default(),
    }
}

#[tauri::command]
pub fn save_settings(state: State<'_, AppState>, settings: AppSettings) -> ApiResult<()> {
    state.db.require_write()?;
    if !(0.5..=64.0).contains(&settings.blur_radius) {
        return Err(ApiError::new("E_VALIDATION", "模糊半径超出范围（0.5–64.0）"));
    }

    let pairs: Vec<(&str, String)> = vec![
        ("output_dir", settings.output_dir.unwrap_or_default()),
        ("blur_radius", format!("{:.2}", settings.blur_radius)),
        (
            "window_width",
            settings.window_width.map(|value| value.to_string()).unwrap_or_default(),
        ),
        (
            "window_height",
            settings.window_height.map(|value| value.to_string()).unwrap_or_default(),
        ),
        ("session_state", settings.session_state.unwrap_or_default()),
    ];

    state.db.with(|conn| {
        let transaction = conn.unchecked_transaction()?;
        let now = now_iso();
        for (key, value) in &pairs {
            transaction.execute(
                "INSERT INTO settings(key, value, updated_at) VALUES(?1, ?2, ?3)
                 ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = excluded.updated_at",
                rusqlite::params![key, value, now],
            )?;
        }
        transaction.commit()?;
        Ok(())
    })
}
