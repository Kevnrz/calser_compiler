"""Puente de E/S entre QProcess y un shell con PTY real (solo Linux)."""
import errno
import os
import pty
import select
import shutil
import signal
import sys
import time


def main():
    shell = shutil.which("bash") or "/bin/sh"
    pid, master = pty.fork()
    if pid == 0:
        os.chdir(sys.argv[1])
        os.environ.update(TERM="dumb", PS1="calser:\\w $ ", PROMPT_COMMAND="", HISTFILE="/dev/null")
        args = [shell, "--noprofile", "--norc", "-i"] if shell.endswith("bash") else [shell, "-i"]
        os.execv(shell, args)
    running = True

    def stop(*_):
        nonlocal running
        running = False

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    try:
        while running:
            ready, _, _ = select.select([master, sys.stdin.fileno()], [], [], .2)
            for fd in ready:
                try:
                    data = os.read(fd, 65536)
                except OSError as error:
                    if error.errno == errno.EIO:
                        return
                    raise
                if not data:
                    return
                target = sys.stdout.fileno() if fd == master else master
                while data:
                    data = data[os.write(target, data):]
    finally:
        # Colgar foreground y shell de esta sesión, sin tocar otras terminales.
        try:
            foreground = os.tcgetpgrp(master)
            if foreground > 0:
                os.killpg(foreground, signal.SIGHUP)
        except (ProcessLookupError, OSError):
            pass
        os.close(master)
        try:
            os.kill(pid, signal.SIGHUP)
        except ProcessLookupError:
            pass
        for _ in range(20):
            try:
                if os.waitpid(pid, os.WNOHANG)[0]:
                    return
            except ChildProcessError:
                return
            time.sleep(.02)
        try:
            os.kill(pid, signal.SIGKILL)
            os.waitpid(pid, 0)
        except (ProcessLookupError, ChildProcessError):
            pass


if __name__ == "__main__":
    main()
