/**
 * @file s_file.c
 * @brief Implementation of system call interface for file I/O operations
 *
 * Implements user-level system call wrappers that translate process-local
 * file descriptors to kernel file descriptors and manage interrupt states.
 * Handles terminal control for stdin reads and delegates to kernel file
 * operations (k_file.h) for actual filesystem access.
 */

#include "s_file.h"

#include "io/filetable/k_filetable.h"
#include "io/path/k_path.h"
#include "io/term/k_term.h"
#include "k_file.h"
#include "process/schedule/k_schedule.h"

#include "io/term/s_term.h"
#include "process/schedule/s_schedule.h"
#include "process/signal/s_signal.h"

/**
 * @brief Opens a file in the fat filesystem
 *
 * @param fname Path to the file in the filesystem
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Process-local file descriptor on success, -1 on error
 * @see k_open for documentation about mode behavior
 */
int s_open(const char* fname, int mode) {
  /* Forward call to k_open */
  k_disable_interrupts();
  int fd = k_open(fname, mode);
  k_enable_interrupts();
  if (fd == -1) {
    return -1;
  }

  /* Convert kernel fd to local fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_put(pid, fd);
    k_enable_interrupts();
  }
  return fd;
}

/**
 * @brief Opens an external host file
 *
 * @param fname Path to the external file
 * @param mode File access mode (F_READ, F_WRITE, F_APPEND, or F_EXEC)
 * @return Process-local file descriptor on success, -1 on error
 * @see h_open for documentation about mode behavior
 */
int s_ext_open(const char* fname, int mode) {
  /* Forward call to k_ext_open */
  k_disable_interrupts();
  int fd = k_ext_open(fname, mode);
  k_enable_interrupts();
  if (fd == -1) {
    return -1;
  }

  /* Convert kernel fd to local fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_put(pid, fd);
    k_enable_interrupts();
  }
  return fd;
}

/**
 * @brief Reads data from a file
 *
 * @param fd Process-local file descriptor
 * @param buf Buffer to store read data
 * @param nbytes Number of bytes to read
 * @return Number of bytes read on success, -1 on error, 0 on EOF
 */
int s_read(int fd, void* buf, size_t nbytes) {
  /* Convert local fd to kernel fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_get(pid, fd);
    k_enable_interrupts();
    if (fd < 0) {
      return -1;
    }
  }

  /* Gate stdin reads by terminal control */
  k_disable_interrupts();
  bool is_term = k_is_term(fd);
  k_enable_interrupts();
  if (is_term) {
    pid_t self = s_getpid();
    /* If this process does not control the terminal, stop until it does */
    while (k_get_terminal_owner() != self) {
      if (s_can_sig_stop(s_getpid())) {
        /* Send stop to self; scheduler will mark STOPPED and suspend */
        s_kill(self, SIGSTOP);
        /* When continued, loop will recheck control and proceed when foreground
         */
      } else {
        /* In the case of an expected infinite hang, just fail */
        return -1;
      }
    }
  }

  /* Redirect to h_ext_read if host fd */
  k_disable_interrupts();
  int hfd = k_fd_get_hfd(fd);
  k_enable_interrupts();
  if (hfd != -1) {
    return k_ext_read(hfd, buf, nbytes);
  }

  /* Redirect to k_read if local fs file */
  k_disable_interrupts();
  int res = k_read(fd, buf, nbytes);
  k_enable_interrupts();
  return res;
}

/**
 * @brief Writes data to a file
 *
 * @param fd Process-local file descriptor
 * @param buf Buffer containing data to write
 * @param nbytes Number of bytes to write
 * @return Number of bytes written on success, -1 on error
 */
int s_write(int fd, const void* buf, size_t nbytes) {
  /* Convert local fd to kernel fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_get(pid, fd);
    k_enable_interrupts();
    if (fd < 0) {
      return -1;
    }
  }

  /* Redirect to k_write */
  k_disable_interrupts();
  int res = k_write(fd, buf, nbytes);
  k_enable_interrupts();
  return res;
}

/**
 * @brief Closes a file descriptor (system call interface)
 *
 * If called from a process context, releases the process-local file
 * descriptor which decrements the reference count on the kernel file
 * descriptor. If called without a process context (pid == -1), directly
 * closes the kernel file descriptor.
 *
 * @param fd Process-local file descriptor to close
 * @return 0 on success, -1 on error
 */
int s_close(int fd) {
  /* If not a process, close from global fd table */
  pid_t pid = k_sched_current_pid();
  if (pid == -1) {
    k_disable_interrupts();
    int res = k_fd_close(fd);
    k_enable_interrupts();
    return res;
  }

  /* If a process, close from local fd table */
  k_disable_interrupts();
  int res = k_lfd_release(pid, fd);
  k_enable_interrupts();
  return res;
}

/**
 * @brief Deletes a file from the filesystem
 *
 * @param fname Path to the file to delete
 * @return 0 on success, -1 on error
 * @see k_unlink for more details
 */
int s_unlink(const char* fname) {
  k_disable_interrupts();
  int res = k_unlink(fname);
  k_enable_interrupts();
  return res;
}

/**
 * @brief Sets the file offset
 *
 * @param fd Process-local file descriptor
 * @param offset Offset value
 * @param whence Reference position (F_SEEK_SET, F_SEEK_CUR, or F_SEEK_END)
 * @see k_lseek for more details
 */
void s_lseek(int fd, int offset, int whence) {
  /* Convert local fd to kernel fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_get(pid, fd);
    k_enable_interrupts();
    if (fd < 0) {
      return;
    }
  }

  /* Redirect to k_lseek */
  k_disable_interrupts();
  k_lseek(fd, offset, whence);
  k_enable_interrupts();
}

/**
 * @brief Renames a file in the filesystem
 *
 * @param source Current file name
 * @param dest New file name
 * @return 0 on success, -1 on error
 * @see k_mv for more details
 */
int s_mv(const char* source, const char* dest) {
  k_disable_interrupts();
  int res = k_mv(source, dest);
  k_enable_interrupts();
  return res;
}

/**
 * @brief Changes the file permissions of the provided file
 *
 * Changes the file permissions of the provided file (1=execute, 2=write,
 * 4=read).
 *
 * @param filename Path to the file
 * @param flag Permission bit to modify (1=execute, 2=write, 4=read)
 * @param add 1 to add permission, 0 to remove permission
 * @return 0 on success, -1 on error (file not found)
 */
int s_chmod(const char* filename, unsigned char flag, int add) {
  extern file_t root_dir;
  file_t file;

  /* Obtain file */
  k_disable_interrupts();
  int result = k_dir_search(filename, &root_dir, &file);
  k_enable_interrupts();
  if (result < 0) {
    return -1;
  }

  /* Change permission */
  if (add == 1) {
    file.perm |= flag;
  } else {
    file.perm &= ~flag;
  }

  /* Update directory entry */
  file.mtime = time(NULL);
  k_disable_interrupts();
  k_dir_upd_entry(&root_dir, result, file);
  k_enable_interrupts();

  return 0;
}

/**
 * @brief Gets the open mode of a file descriptor
 *s
 * @param fd Process-local file descriptor
 * @return Open mode on success, -1 on error
 */
int s_get_open_mode(int fd) {
  /* Convert local fd to kernel fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd = k_lfd_get(pid, fd);
    k_enable_interrupts();
    if (fd < 0) {
      return -1;
    }
  }

  /* Redirect to k_fd_mode */
  return k_fd_mode(fd);
}

/**
 * @brief Checks if two file descriptors refer to the same file
 *
 * @param fd1 First process-local file descriptor
 * @param fd2 Second process-local file descriptor
 * @return true if both refer to the same file, false otherwise
 */
bool s_same_source(int fd1, int fd2) {
  /* Convert local fd to kernel fd */
  pid_t pid = k_sched_current_pid();
  if (pid != -1) {
    k_disable_interrupts();
    fd1 = k_lfd_get(pid, fd1);
    k_enable_interrupts();
    if (fd1 < 0) {
      return false;
    }
    k_disable_interrupts();
    fd2 = k_lfd_get(pid, fd2);
    k_enable_interrupts();
    if (fd2 < 0) {
      return false;
    }
  }

  /* Check the sources are the same */
  k_disable_interrupts();
  bool same = false;
  if (k_fd_get_hfd(fd1) != -1) {
    /* If fd1 is hfd, check if it is the same as hfd of fd2 (which could be -1)
     */
    same = k_fd_get_hfd(fd1) == k_fd_get_hfd(fd2);
  } else if (k_fd_get_hfd(fd2) == -1) {
    /* Both fd1 and fd2 are local files, check if they are equal */
    /* The pointer to the file data should be an exact match */
    same = k_fd_file_ref(fd1) == k_fd_file_ref(fd2);
  }
  k_enable_interrupts();

  return same;
}