/* Copyright (c) 2020,  Queen's Printer for Ontario */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Log.h"
namespace tbd::logging
{
int Log::logging_level_ = LOG_DEBUG;
// do this in .cpp so that we don't get unused warnings including the .h
static const char* LOG_LABELS[] =
  {
    "EXTENSIVE: ",
    "VERBOSE:   ",
    "DEBUG:     ",
    "INFO:      ",
    "NOTE:      ",
    "WARNING:   ",
    "ERROR:     ",
    "FATAL:     ",
    "SILENT:    "};
mutex mutex_;
void Log::setLogLevel(const int log_level) noexcept
{
  logging_level_ = log_level;
}
void Log::increaseLogLevel() noexcept
{
  // HACK: make sure we never go below 0
  logging_level_ = max(0, getLogLevel() - 1);
}
void Log::decreaseLogLevel() noexcept
{
  // HACK: make sure we never go above silent
  logging_level_ = min(LOG_SILENT, getLogLevel() + 1);
}
int Log::getLogLevel() noexcept
{
  return logging_level_;
}
static FILE* out_;
int Log::openLogFile(const char* filename) noexcept
{
  out_ = fopen(filename, "w");
  return nullptr != out_;
}
int Log::closeLogFile() noexcept
{
  return fclose(out_);
}
void output(const int log_level, const char* format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
  if (Log::getLogLevel() > log_level)
  {
    return;
  }
  try
  {
    // NOTE: create string first so that entire line writes
    // (otherwise threads might mix lines)
    const string tmp;
    stringstream iss(tmp);
#ifdef NDEBUG
    const time_t now = time(nullptr);
    auto buf = localtime(&now);
    iss << put_time(buf, "[%F %T] ");
#endif
    // try to make output consistent if in debug mode
    iss << LOG_LABELS[log_level];
    static char buffer[1024]{0};
    vsprintf(buffer, format, *args);
    iss << buffer;
    {
      lock_guard<mutex> lock(mutex_);
      printf("%s\n", iss.str().c_str());
      if (nullptr != out_)
      {
        fprintf(out_, "%s\n", iss.str().c_str());
      }
    }
  }
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
    std::terminate();
  }
}
void output(const int log_level, const char* format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  output(log_level, format, &args);
  va_end(args);
}
void extensive(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_EXTENSIVE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_EXTENSIVE, format, &args);
    va_end(args);
  }
}
void verbose(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_VERBOSE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_VERBOSE, format, &args);
    va_end(args);
  }
}
void debug(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_DEBUG)
  {
    va_list args;
    va_start(args, format);
    output(LOG_DEBUG, format, &args);
    va_end(args);
  }
}
void info(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_INFO)
  {
    va_list args;
    va_start(args, format);
    output(LOG_INFO, format, &args);
    va_end(args);
  }
}
void note(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_NOTE)
  {
    va_list args;
    va_start(args, format);
    output(LOG_NOTE, format, &args);
    va_end(args);
  }
}
void warning(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_WARNING)
  {
    va_list args;
    va_start(args, format);
    output(LOG_WARNING, format, &args);
    va_end(args);
  }
}
void error(const char* format, ...) noexcept
{
  if (Log::getLogLevel() <= LOG_ERROR)
  {
    va_list args;
    va_start(args, format);
    output(LOG_ERROR, format, &args);
    va_end(args);
  }
}
void fatal(const char* format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
  // HACK: call the other version
  fatal<int>(format, args);
}
void fatal(const char* format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  fatal(format, &args);
  // cppcheck-suppress va_end_missing
  // va_end(args);
}
void fatal(const std::exception& ex)
{
  output(LOG_FATAL, "%s", ex.what());
  Log::closeLogFile();
#ifdef NDEBUG
  exit(EXIT_FAILURE);
#endif
}
void fatal(const std::exception& ex, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  output(LOG_FATAL, format, &args);
  // cppcheck-suppress va_end_missing
  // va_end(args);
  fatal(ex);
}
void check_fatal(const bool condition, const char* format, va_list* args)
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    fatal(format, args);
  }
}
void check_fatal(const bool condition, const char* format, ...)
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    va_list args;
    va_start(args, format);
    fatal(format, &args);
    // cppcheck-suppress va_end_missing
    // va_end(args);
  }
}

void SelfLogger::log_extensive(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_EXTENSIVE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_verbose(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_VERBOSE, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_debug(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_DEBUG, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_info(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_INFO, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_note(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_NOTE, fmt.c_str(), &args);
  va_end(args);
}

void SelfLogger::log_warning(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_WARNING, fmt.c_str(), &args);
  va_end(args);
}

void SelfLogger::log_error(const char* format, ...) const noexcept
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::output(LOG_ERROR, fmt.c_str(), &args);
  va_end(args);
}
void SelfLogger::log_check_fatal(bool condition, const char* format, ...) const
#ifdef NDEBUG
  noexcept
#endif
{
  if (condition)
  {
    va_list args;
    va_start(args, format);
    const auto fmt = add_log(format);
    logging::fatal(fmt.c_str(), &args);
    va_end(args);
  }
}

void SelfLogger::log_fatal(const char* format, ...) const
#ifdef NDEBUG
  noexcept
#endif
{
  va_list args;
  va_start(args, format);
  const auto fmt = add_log(format);
  logging::fatal(fmt.c_str(), &args);
  va_end(args);
}
}
