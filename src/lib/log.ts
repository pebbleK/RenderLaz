export type LogLevel = "info" | "success" | "warn" | "error";

export type LogEntry = {
  id: number;
  time: string;
  level: LogLevel;
  message: string;
};

type Listener = (entry: LogEntry) => void;

const listeners = new Set<Listener>();
let sequence = 0;

function timestamp(): string {
  const now = new Date();
  const pad = (value: number) => String(value).padStart(2, "0");
  return `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`;
}

export function log(level: LogLevel, message: string): void {
  const entry: LogEntry = { id: ++sequence, time: timestamp(), level, message };
  for (const listener of listeners) {
    listener(entry);
  }
}

export const logger = {
  info: (message: string) => log("info", message),
  success: (message: string) => log("success", message),
  warn: (message: string) => log("warn", message),
  error: (message: string) => log("error", message),
};

export function subscribeLog(listener: Listener): () => void {
  listeners.add(listener);
  return () => {
    listeners.delete(listener);
  };
}
