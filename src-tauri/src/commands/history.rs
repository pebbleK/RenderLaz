use std::collections::HashMap;

use rusqlite::params;
use tauri::State;

use crate::db::now_iso;
use crate::error::{ApiError, ApiResult};
use crate::models::{
    BatchItem, BatchRun, BatchRunResult, ItemStatus, RunStatus,
};
use crate::AppState;

#[tauri::command]
pub fn start_batch_run(
    state: State<'_, AppState>,
    input_count: i64,
    output_dir: Option<String>,
) -> ApiResult<i64> {
    state.db.require_write()?;
    if input_count < 0 {
        return Err(ApiError::new("E_VALIDATION", "输入数量不能为负"));
    }

    state.db.with(|conn| {
        conn.execute(
            "INSERT INTO batch_runs(started_at, input_count, output_dir, status)
             VALUES(?1, ?2, ?3, ?4)",
            params![now_iso(), input_count, output_dir, RunStatus::Running.as_str()],
        )?;
        Ok(conn.last_insert_rowid())
    })
}

#[tauri::command]
pub fn finish_batch_run(state: State<'_, AppState>, result: BatchRunResult) -> ApiResult<()> {
    state.db.require_write()?;

    let counted = result
        .items
        .iter()
        .filter(|item| item.status != ItemStatus::Skipped)
        .count() as i64;
    if counted != result.ok_count + result.fail_count {
        return Err(ApiError::new("E_VALIDATION", "批次计数与条目明细不一致")
            .with_context(format!("明细 {counted}，成功+失败 {}", result.ok_count + result.fail_count)));
    }

    state.db.with(|conn| {
        let transaction = conn.unchecked_transaction()?;
        let updated = transaction.execute(
            "UPDATE batch_runs
             SET finished_at = ?1, status = ?2, ok_count = ?3, fail_count = ?4
             WHERE id = ?5",
            params![
                now_iso(),
                result.status.as_str(),
                result.ok_count,
                result.fail_count,
                result.id
            ],
        )?;
        if updated == 0 {
            return Err(ApiError::new("E_NOT_FOUND", "批次不存在").with_context(result.id.to_string()));
        }

        for item in &result.items {
            transaction.execute(
                "INSERT INTO batch_items(run_id, path, output_path, status, error)
                 VALUES(?1, ?2, ?3, ?4, ?5)",
                params![result.id, item.path, item.output_path, item.status.as_str(), item.error],
            )?;
        }

        transaction.commit()?;
        Ok(())
    })
}

#[tauri::command]
pub fn list_batch_runs(state: State<'_, AppState>, limit: i64) -> ApiResult<Vec<BatchRun>> {
    let limit = limit.clamp(1, 50);

    state.db.with(|conn| {
        let mut statement = conn.prepare(
            "SELECT id, started_at, finished_at, input_count, ok_count, fail_count, output_dir, status
             FROM batch_runs ORDER BY started_at DESC, id DESC LIMIT ?1",
        )?;

        let mut runs: Vec<BatchRun> = statement
            .query_map([limit], |row| {
                let status: String = row.get(7)?;
                Ok(BatchRun {
                    id: row.get(0)?,
                    started_at: row.get(1)?,
                    finished_at: row.get(2)?,
                    input_count: row.get(3)?,
                    ok_count: row.get(4)?,
                    fail_count: row.get(5)?,
                    output_dir: row.get(6)?,
                    status: RunStatus::parse(&status).unwrap_or(RunStatus::Failed),
                    items: Vec::new(),
                })
            })?
            .collect::<Result<Vec<_>, _>>()?;

        if runs.is_empty() {
            return Ok(runs);
        }

        let mut item_statement = conn.prepare(
            "SELECT run_id, path, output_path, status, error FROM batch_items ORDER BY id",
        )?;
        let mut grouped: HashMap<i64, Vec<BatchItem>> = HashMap::new();
        let rows = item_statement.query_map([], |row| {
            let run_id: i64 = row.get(0)?;
            let status: String = row.get(3)?;
            Ok((
                run_id,
                BatchItem {
                    path: row.get(1)?,
                    output_path: row.get(2)?,
                    status: ItemStatus::parse(&status).unwrap_or(ItemStatus::Skipped),
                    error: row.get(4)?,
                },
            ))
        })?;
        for row in rows {
            let (run_id, item) = row?;
            grouped.entry(run_id).or_default().push(item);
        }

        for run in &mut runs {
            if let Some(items) = grouped.remove(&run.id) {
                run.items = items;
            }
        }

        Ok(runs)
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::models::BatchItemInput;

    fn memory_conn() -> rusqlite::Connection {
        let conn = rusqlite::Connection::open_in_memory().unwrap();
        conn.execute_batch(crate::db::SCHEMA_SQL).unwrap();
        conn
    }

    #[test]
    fn item_counts_must_match_result() {
        let result = BatchRunResult {
            id: 1,
            status: RunStatus::Completed,
            ok_count: 2,
            fail_count: 0,
            items: vec![BatchItemInput {
                path: "a.png".into(),
                output_path: Some("out/a.png".into()),
                status: ItemStatus::Ok,
                error: None,
            }],
        };
        let counted = result.items.iter().filter(|item| item.status != ItemStatus::Skipped).count() as i64;
        assert_ne!(counted, result.ok_count + result.fail_count);
    }

    #[test]
    fn schema_accepts_finished_run_with_items() {
        let conn = memory_conn();
        conn.execute(
            "INSERT INTO batch_runs(started_at, input_count, status) VALUES('2026-11-14T10:00:00+08:00', 2, 'running')",
            [],
        )
        .unwrap();
        let id = conn.last_insert_rowid();
        conn.execute(
            "UPDATE batch_runs SET finished_at = '2026-11-14T10:01:00+08:00', status = 'completed', ok_count = 1, fail_count = 1 WHERE id = ?1",
            [id],
        )
        .unwrap();
        conn.execute(
            "INSERT INTO batch_items(run_id, path, status) VALUES(?1, 'a.png', 'ok'), (?1, 'b.png', 'failed')",
            [id],
        )
        .unwrap();

        let count: i64 = conn
            .query_row("SELECT COUNT(*) FROM batch_items WHERE run_id = ?1", [id], |row| row.get(0))
            .unwrap();
        assert_eq!(count, 2);
    }
}
