use serde::{Deserialize, Serialize};
use ts_rs::TS;

const EXPORT_TO: &str = "../../src/lib/bindings/";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, TS)]
#[serde(rename_all = "lowercase")]
#[ts(export, export_to = EXPORT_TO)]
pub enum EffectKind {
    Null,
    Grayscale,
    Invert,
    Sepia,
    Blur,
}

impl EffectKind {
    pub fn as_str(&self) -> &'static str {
        match self {
            EffectKind::Null => "null",
            EffectKind::Grayscale => "grayscale",
            EffectKind::Invert => "invert",
            EffectKind::Sepia => "sepia",
            EffectKind::Blur => "blur",
        }
    }

    pub fn parse(value: &str) -> Option<Self> {
        match value {
            "null" => Some(EffectKind::Null),
            "grayscale" => Some(EffectKind::Grayscale),
            "invert" => Some(EffectKind::Invert),
            "sepia" => Some(EffectKind::Sepia),
            "blur" => Some(EffectKind::Blur),
            _ => None,
        }
    }
}

/// 每个特效步骤都可调的参数。CPU 特效只用 amount / contrast，
/// blur 额外用 radius / iterations；未使用的字段原样保存。
#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct EffectParams {
    /// 与上一步结果的混合比例，1.0 表示完全采用本特效
    pub amount: f32,
    /// 对比度，1.0 表示不变
    pub contrast: f32,
    /// 模糊半径，仅 blur 使用
    pub radius: f32,
    /// 模糊迭代次数，仅 blur 使用
    pub iterations: i32,
}

impl Default for EffectParams {
    fn default() -> Self {
        Self { amount: 1.0, contrast: 1.0, radius: 8.0, iterations: 1 }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct EffectStep {
    pub kind: EffectKind,
    pub params: EffectParams,
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct ImportOutcome {
    pub path: String,
    pub name: String,
    pub ok: bool,
    pub error: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct AppSettings {
    pub output_dir: Option<String>,
    pub blur_radius: f32,
    #[ts(type = "number | null")]
    pub window_width: Option<i64>,
    #[ts(type = "number | null")]
    pub window_height: Option<i64>,
    /// 会话恢复用 JSON 字符串，不落盘为独立文件。
    pub session_state: Option<String>,
}

impl Default for AppSettings {
    fn default() -> Self {
        Self {
            output_dir: None,
            blur_radius: 8.0,
            window_width: None,
            window_height: None,
            session_state: None,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, TS)]
#[serde(rename_all = "lowercase")]
#[ts(export, export_to = EXPORT_TO)]
pub enum RunStatus {
    Running,
    Completed,
    Canceled,
    Failed,
}

impl RunStatus {
    pub fn as_str(&self) -> &'static str {
        match self {
            RunStatus::Running => "running",
            RunStatus::Completed => "completed",
            RunStatus::Canceled => "canceled",
            RunStatus::Failed => "failed",
        }
    }

    pub fn parse(value: &str) -> Option<Self> {
        match value {
            "running" => Some(RunStatus::Running),
            "completed" => Some(RunStatus::Completed),
            "canceled" => Some(RunStatus::Canceled),
            "failed" => Some(RunStatus::Failed),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, TS)]
#[serde(rename_all = "lowercase")]
#[ts(export, export_to = EXPORT_TO)]
pub enum ItemStatus {
    Ok,
    Failed,
    Skipped,
}

impl ItemStatus {
    pub fn as_str(&self) -> &'static str {
        match self {
            ItemStatus::Ok => "ok",
            ItemStatus::Failed => "failed",
            ItemStatus::Skipped => "skipped",
        }
    }

    pub fn parse(value: &str) -> Option<Self> {
        match value {
            "ok" => Some(ItemStatus::Ok),
            "failed" => Some(ItemStatus::Failed),
            "skipped" => Some(ItemStatus::Skipped),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct BatchItemInput {
    pub path: String,
    pub output_path: Option<String>,
    pub status: ItemStatus,
    pub error: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct BatchRunResult {
    #[ts(type = "number")]
    pub id: i64,
    pub status: RunStatus,
    #[ts(type = "number")]
    pub ok_count: i64,
    #[ts(type = "number")]
    pub fail_count: i64,
    pub items: Vec<BatchItemInput>,
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct BatchItem {
    pub path: String,
    pub output_path: Option<String>,
    pub status: ItemStatus,
    pub error: Option<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize, TS)]
#[ts(export, export_to = EXPORT_TO)]
pub struct BatchRun {
    #[ts(type = "number")]
    pub id: i64,
    pub started_at: String,
    pub finished_at: Option<String>,
    #[ts(type = "number")]
    pub input_count: i64,
    #[ts(type = "number")]
    pub ok_count: i64,
    #[ts(type = "number")]
    pub fail_count: i64,
    pub output_dir: Option<String>,
    pub status: RunStatus,
    pub items: Vec<BatchItem>,
}

