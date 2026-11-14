use std::path::Path;
use std::sync::Mutex;

use chrono::Local;
use rusqlite::Connection;

use crate::error::{ApiError, ApiResult};

const SCHEMA_VERSION: i64 = 2;

pub(crate) const SCHEMA_SQL: &str = r#"
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
PRAGMA busy_timeout = 5000;

CREATE TABLE IF NOT EXISTS schema_meta (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS settings (
    key        TEXT PRIMARY KEY,
    value      TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS batch_runs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    started_at  TEXT    NOT NULL,
    finished_at TEXT,
    input_count INTEGER NOT NULL,
    ok_count    INTEGER NOT NULL DEFAULT 0,
    fail_count  INTEGER NOT NULL DEFAULT 0,
    output_dir  TEXT,
    status      TEXT    NOT NULL
);

CREATE TABLE IF NOT EXISTS batch_items (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id      INTEGER NOT NULL REFERENCES batch_runs(id) ON DELETE CASCADE,
    path        TEXT    NOT NULL,
    output_path TEXT,
    status      TEXT    NOT NULL,
    error       TEXT
);

CREATE INDEX IF NOT EXISTS idx_batch_items_run           ON batch_items(run_id);
CREATE INDEX IF NOT EXISTS idx_batch_runs_started        ON batch_runs(started_at DESC);
"#;

/// 单连接 + Mutex。读写在命令层串行，避免并发写锁。
pub struct Db {
    conn: Mutex<Connection>,
    pub read_only: bool,
    pub warning: Option<String>,
}

impl Db {
    pub fn open(path: &Path) -> ApiResult<Self> {
        match Self::open_rw(path) {
            Ok(db) => Ok(db),
            Err(rw_error) => {
                // 损坏优先于只读：能打开但完整性不足时备份后重建。
                if path.exists() {
                    if let Some(backup) = quarantine(path) {
                        if let Ok(db) = Self::open_rw(path) {
                            return Ok(Db {
                                warning: Some(format!(
                                    "数据库已损坏，原文件备份为 {}，已新建空库",
                                    backup
                                )),
                                ..db
                            });
                        }
                    }
                }
                // 最后兜底：内存库，只读模式，应用仍可启动。
                let conn = Connection::open_in_memory().map_err(|err| {
                    ApiError::new("E_DB", format!("无法打开数据库：{err}"))
                        .with_context(path.display().to_string())
                })?;
                conn.execute_batch(SCHEMA_SQL)?;
                Ok(Db {
                    conn: Mutex::new(conn),
                    read_only: true,
                    warning: Some(format!(
                        "数据库不可用（{}），本次以只读模式运行，改动不会保存",
                        rw_error.message
                    )),
                })
            }
        }
    }

    fn open_rw(path: &Path) -> ApiResult<Self> {
        let conn = Connection::open(path)?;
        conn.execute_batch(SCHEMA_SQL)?;

        let check: String = conn.query_row("PRAGMA quick_check", [], |row| row.get(0))?;
        if check != "ok" {
            return Err(ApiError::new("E_DB", "数据库完整性检查未通过")
                .with_context(path.display().to_string()));
        }

        // 迁移：v1 的工程三表在 v2 中退役（工程保存功能已移除）。
        let current_version: i64 = conn
            .query_row(
                "SELECT value FROM schema_meta WHERE key = 'schema_version'",
                [],
                |row| row.get::<_, String>(0),
            )
            .ok()
            .and_then(|value| value.parse().ok())
            .unwrap_or(0);

        if current_version < 2 {
            conn.execute_batch(
                "DROP TABLE IF EXISTS project_resources;
                 DROP TABLE IF EXISTS project_effects;
                 DROP TABLE IF EXISTS projects;",
            )?;
        }

        conn.execute(
            "INSERT INTO schema_meta(key, value) VALUES('schema_version', ?1)
             ON CONFLICT(key) DO UPDATE SET value = excluded.value",
            [SCHEMA_VERSION.to_string()],
        )?;

        let now = now_iso();
        conn.execute(
            "UPDATE batch_runs SET status = 'failed', finished_at = ?1 WHERE status = 'running'",
            [now],
        )?;

        Ok(Db { conn: Mutex::new(conn), read_only: false, warning: None })
    }

    pub fn with<T>(&self, f: impl FnOnce(&Connection) -> ApiResult<T>) -> ApiResult<T> {
        let conn = self
            .conn
            .lock()
            .map_err(|_| ApiError::new("E_DB", "数据库连接状态异常"))?;
        f(&conn)
    }

    pub fn require_write(&self) -> ApiResult<()> {
        if self.read_only {
            return Err(ApiError::new("E_DB", "数据库当前为只读状态，无法保存改动"));
        }
        Ok(())
    }
}

fn quarantine(path: &Path) -> Option<String> {
    let stamp = Local::now().format("%Y%m%d%H%M%S");
    let backup = path.with_file_name(format!(
        "{}.corrupt-{stamp}",
        path.file_name()?.to_string_lossy()
    ));
    std::fs::rename(path, &backup).ok()?;
    for suffix in ["-wal", "-shm"] {
        let side = path.with_file_name(format!("{}{suffix}", path.file_name()?.to_string_lossy()));
        let _ = std::fs::remove_file(side);
    }
    Some(backup.display().to_string())
}

pub fn now_iso() -> String {
    Local::now().to_rfc3339_opts(chrono::SecondsFormat::Secs, false)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn creates_schema_and_clears_running_batches() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("renderlaz.db");
        let db = Db::open(&path).unwrap();
        assert!(!db.read_only);

        db.with(|conn| {
            conn.execute(
                "INSERT INTO batch_runs(started_at, input_count, status) VALUES(?1, 1, 'running')",
                [now_iso()],
            )?;
            Ok(())
        })
        .unwrap();

        // 重新打开等同于一次启动，running 应被清理为 failed
        drop(db);
        let db = Db::open(&path).unwrap();
        let status: String = db
            .with(|conn| Ok(conn.query_row("SELECT status FROM batch_runs", [], |r| r.get(0))?))
            .unwrap();
        assert_eq!(status, "failed");
    }

    #[test]
    fn drops_legacy_project_tables() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("renderlaz.db");

        // 模拟 v1 库：有工程表与一行数据
        {
            let conn = Connection::open(&path).unwrap();
            conn.execute_batch(
                "CREATE TABLE schema_meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
                 INSERT INTO schema_meta(key, value) VALUES('schema_version', '1');
                 CREATE TABLE projects (id INTEGER PRIMARY KEY, name TEXT NOT NULL, created_at TEXT NOT NULL, updated_at TEXT NOT NULL);
                 INSERT INTO projects(name, created_at, updated_at) VALUES('旧工程', 'x', 'x');",
            )
            .unwrap();
        }

        let db = Db::open(&path).unwrap();
        let (projects_table, version): (i64, String) = db
            .with(|conn| {
                let count: i64 = conn.query_row(
                    "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'projects'",
                    [],
                    |row| row.get(0),
                )?;
                let version: String =
                    conn.query_row("SELECT value FROM schema_meta WHERE key = 'schema_version'", [], |row| row.get(0))?;
                Ok((count, version))
            })
            .unwrap();

        assert_eq!(projects_table, 0, "旧工程表应被迁移删除");
        assert_eq!(version, "2");
    }

    #[test]
    fn corrupt_file_is_quarantined() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("renderlaz.db");
        std::fs::write(&path, b"not a database at all").unwrap();

        let db = Db::open(&path).unwrap();
        assert!(db.warning.is_some());
        let backups: Vec<_> = std::fs::read_dir(dir.path())
            .unwrap()
            .filter_map(|e| e.ok())
            .filter(|e| e.file_name().to_string_lossy().contains(".corrupt-"))
            .collect();
        assert_eq!(backups.len(), 1, "损坏库应被备份");
    }
}
