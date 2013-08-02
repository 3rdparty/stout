#ifndef __STOUT_OS_PROCESS_HPP__
#define __STOUT_OS_PROCESS_HPP__

#include <sys/types.h> // For pid_t.

#include <string>

#include <stout/bytes.hpp>
#include <stout/duration.hpp>
#include <stout/option.hpp>

namespace os {

struct Process
{
  Process(pid_t _pid,
          pid_t _parent,
          pid_t _group,
          pid_t _session,
          const Option<Bytes>& _rss,
          const Option<Duration>& _utime,
          const Option<Duration>& _stime,
          const std::string& _command,
          bool _zombie)
    : pid(_pid),
      parent(_parent),
      group(_group),
      session(_session),
      rss(_rss),
      utime(_utime),
      stime(_stime),
      command(_command),
      zombie(_zombie) {}

  const pid_t pid;
  const pid_t parent;
  const pid_t group;
  const pid_t session;
  const Option<Bytes> rss;
  const Option<Duration> utime;
  const Option<Duration> stime;
  const std::string command;
  const bool zombie;

  // TODO(bmahler): Add additional data as needed.

  bool operator <  (const Process& p) const { return pid <  p.pid; }
  bool operator <= (const Process& p) const { return pid <= p.pid; }
  bool operator >  (const Process& p) const { return pid >  p.pid; }
  bool operator >= (const Process& p) const { return pid >= p.pid; }
  bool operator == (const Process& p) const { return pid == p.pid; }
  bool operator != (const Process& p) const { return pid != p.pid; }
};

} // namespace os {

#endif // __STOUT_OS_PROCESS_HPP__
