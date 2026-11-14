mod commands;
mod db;
mod error;
mod models;
mod paths;

use tauri::Manager;

use crate::db::Db;

pub struct AppState {
    pub db: Db,
}

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            let path = paths::db_path(app.handle())?;
            let database = Db::open(&path)?;
            if let Some(warning) = &database.warning {
                eprintln!("[renderlaz] {warning}");
            }
            app.manage(AppState { db: database });
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::images::import_images,
            commands::images::read_image,
            commands::images::ensure_dir,
            commands::images::write_png,
            commands::settings::get_settings,
            commands::settings::save_settings,
            commands::history::start_batch_run,
            commands::history::finish_batch_run,
            commands::history::list_batch_runs,
            commands::system::database_status,
        ])
        .run(tauri::generate_context!())
        .expect("RenderLaz 启动失败");
}
