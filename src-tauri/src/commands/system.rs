use serde::Serialize;
use tauri::State;
use ts_rs::TS;

use crate::AppState;

/// 数据库运行状态：告警文本与是否只读，供界面提示（FR-023）。
#[derive(Debug, Clone, Serialize, TS)]
#[ts(export, export_to = "../../src/lib/bindings/")]
pub struct DatabaseStatus {
    pub warning: Option<String>,
    pub read_only: bool,
}

#[tauri::command]
pub fn database_status(state: State<'_, AppState>) -> DatabaseStatus {
    DatabaseStatus {
        warning: state.db.warning.clone(),
        read_only: state.db.read_only,
    }
}
