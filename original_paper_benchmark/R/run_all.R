args <- commandArgs(trailingOnly = TRUE)

repo_r_dir <- normalizePath(
  if (length(args) >= 1) args[[1]] else ".",
  mustWork = TRUE
)

max_parallel <- 15L
log_dir <- file.path(repo_r_dir, "run_all_logs")
dir.create(log_dir, showWarnings = FALSE, recursive = TRUE)

r_files <- list.files(
  repo_r_dir,
  pattern = "\\.[Rr]$",
  recursive = TRUE,
  full.names = TRUE
)

r_files <- sort(r_files)
r_files <- r_files[!basename(r_files) %in% c("common.R", "run_all.R")]

resolve_data_file <- function(script_path, data_path) {
  if (grepl("^(/|[A-Za-z]:)", data_path)) {
    candidates <- data_path
  } else {
    candidates <- file.path(dirname(script_path), data_path)
  }

  if (endsWith(data_path, ".csv.gz")) {
    candidates <- c(candidates, sub("\\.gz$", "", candidates))
  } else if (endsWith(data_path, ".csv")) {
    candidates <- c(candidates, paste0(candidates, ".gz"))
  }

  candidates[file.exists(candidates)][1]
}

csv_refs_for <- function(path) {
  lines <- readLines(path, warn = FALSE)
  lines <- sub("#.*$", "", lines)
  text <- paste(lines, collapse = "\n")
  matches <- gregexpr("['\"][^'\"]+\\.csv(\\.gz)?['\"]", text, perl = TRUE)
  refs <- regmatches(text, matches)[[1]]

  if (!length(refs) || identical(refs, character(0))) {
    character(0)
  } else {
    refs <- unique(gsub("^['\"]|['\"]$", "", refs))
    refs[!grepl("[*()\\[\\]{}]", refs, perl = TRUE)]
  }
}

csv_status <- lapply(r_files, function(path) {
  refs <- csv_refs_for(path)
  missing <- refs[vapply(refs, function(ref) {
    is.na(resolve_data_file(path, ref))
  }, logical(1))]

  list(path = path, refs = refs, missing = missing)
})

skipped <- Filter(function(x) length(x$missing) > 0, csv_status)
no_csv <- Filter(function(x) length(x$refs) == 0, csv_status)
r_files <- vapply(
  Filter(function(x) length(x$refs) > 0 && length(x$missing) == 0, csv_status),
  `[[`,
  character(1),
  "path"
)

if (!length(r_files)) {
  stop("No runnable .R files found below ", repo_r_dir)
}

make_rel_path <- function(path) {
  sub(paste0("^", normalizePath(repo_r_dir), "/?"), "", normalizePath(path))
}

make_log_path <- function(rel_path) {
  safe_name <- gsub("[/\\\\]", "__", rel_path)
  file.path(log_dir, paste0(safe_name, ".log"))
}

run_one <- function(path) {
  script_dir <- dirname(path)
  script_name <- basename(path)
  rel_path <- make_rel_path(path)
  log_path <- make_log_path(rel_path)

  old_wd <- getwd()
  on.exit(setwd(old_wd), add = TRUE)
  setwd(script_dir)

  message("==> Running ", rel_path)

  status <- tryCatch(
    system2(
      command = "Rscript",
      args = script_name,
      stdout = log_path,
      stderr = log_path,
      wait = TRUE
    ),
    error = function(e) {
      writeLines(
        c("Runner error:", conditionMessage(e)),
        con = log_path
      )
      999L
    }
  )

  list(
    path = rel_path,
    log = log_path,
    status = status
  )
}

worker_count <- min(max_parallel, length(r_files))

message("==> Found ", length(r_files), " R scripts")
if (length(skipped)) {
  message("==> Skipping ", length(skipped), " R scripts with missing CSV inputs")
  for (entry in skipped) {
    message(" - ", make_rel_path(entry$path), ": missing ", paste(entry$missing, collapse = ", "))
  }
}
if (length(no_csv)) {
  message("==> Skipping ", length(no_csv), " R scripts without CSV inputs")
  for (entry in no_csv) {
    message(" - ", make_rel_path(entry$path))
  }
}
message("==> Running up to ", worker_count, " scripts in parallel")
message("==> Logs: ", log_dir)
message("")

results <- parallel::mclapply(
  r_files,
  run_one,
  mc.cores = worker_count
)

failed <- Filter(function(x) x$status != 0, results)
passed <- Filter(function(x) x$status == 0, results)

message("")
message("==> Summary")
message("Passed: ", length(passed))
message("Failed: ", length(failed))

if (length(failed)) {
  message("")
  message("Failed scripts:")
  for (entry in failed) {
    message(" - ", entry$path, " (exit ", entry$status, ")")
    message("   log: ", entry$log)
  }
  quit(status = 1)
}
