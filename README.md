# qotd
QOTD (quote of the day) is specified in [RFC 865](https://tools.ietf.org/html/rfc865) as a way of broadcasting a quote to users. On both TCP and UDP, port 17 is officially reserved for this purpose. This program is meant to provide a simple QOTD daemon on IPv4 and IPv6 over TCP/IP. See also [here](https://en.wikipedia.org/wiki/QOTD).

### Installation from package
This program is available on the AUR with both [stable](https://aur.archlinux.org/packages/qotd) and [development](https://aur.archlinux.org/packages/qotd-git) unstable releases.

### Installation from source
Clone this repo, and in the top level directory, run:

```
$ make
# make install
```

This will install the following files on your system:

* `/etc/qotd.conf`
* `/usr/bin/qotdd`
* `/usr/share/qotd/quotes.txt`

If you use _systemd_, install with `make install SYSTEMD=1`. This will add a QOTD service file to your system at `/usr/lib/systemd/system/qotd.service`.

If you're creating a package, you can have `make` install to the packaging directory by setting `ROOT`, e.g. `make install ROOT=/tmp/my_package`.

### Configuration
The default configuration file is located at `/etc/qotd.conf` (though this can be changed with the `-c` flag). The following options are supported:

**TransportProtocol**:  
  Which transport protocol to use. This setting is either 'udp' or 'tcp'.  
  Default: TCP

**InternetProtocol**:  
  Which IP protocol to use. This setting is either 'ipv4', 'ipv6', or 'both'.  
  Default: Both

**Port**:  
  Specifies an alternate port to listen on.  
  Default: 17

**DropPrivileges**:  
  Takes a boolean. If this option is set, then once a connection is established, the program will drop
  privileges and run itself as the daemon user.  
  Default: yes

**PidFile**:  
  Specifies what pid file is to be used by the daemon. If this value is `none` or `/dev/null`, then no pid
  file is written.  
  Default: `/run/qotd.pid`, but if there is no `/run`, then `/var/run/qotd.pid` is used instead.

**RequirePidFile**:  
  If the daemon is unable to write to the pid file for any reason and this option is set, then the daemon
  will quit.  
  Default: yes

**JournalFile**:  
  This option specifies what file the daemon uses to log status messages. If this file is set to `-`, then
  the program's standard output is used, and if the value is set to `none` or `/dev/null`, then the journal
  output is suppressed.  
  Default: the program's standard output

**QuotesFile**:  
  The source  of  the quotations to be displayed to the user. Note that any null bytes (`\0`) found in
  the quotes file will be read as spaces instead. The default is to use the pre-installed quotes located
  at `/usr/share/qotd/quotes.txt`.

**QuoteDivider**:  
  How quotes in the quotes file are separated. There are currently three possible options: 'line',
  'percent', or 'file'. If the value is 'line', then each non-empty line is treated as a quotation to
  be possibly transmitted. If 'percent' is set, then the program is instructed to separate each quote
  with a line that has only a percent sign (%) on it. More specifically, the program looks for a
  sequence of newline, percent sign, and newline, and separates the string there. If 'file' is used, then
  the whole file is treated as one quote.  
  Default: line

**DailyQuotes**:  
  Whether to choose a random quote every day, or for every visit. If this option is set, then the same
  randomly-chosen quotation will be provided for all visits on the same day. Otherwise, each visit will
  yield a different quotation.
  Default: yes

**AllowBigQuotes**:
  RFC 865 specifies  that quotes should be no bigger than 512 bytes. If this option is set, then this
  limit is ignored. Otherwise, quotes are automatically truncated to meet the byte limit.  
  Default: no

### Running
_systemd_ users can run the new service:

```
# systemctl enable qotd.service
# systemctl start qotd.service
```

Those running the daemon directly should be aware of its options:

```
qotd - A simple QOTD daemon.
Usage: qotdd [OPTIONS]...
Usage: qotdd [--help | --version]

  -f, --foreground      Do not fork, but run in the foreground.
  -c, --config &lt;file&gt;   Specify an alternate configuration file location. The default
                        is at "/etc/qotd.conf".
  -N, --noconfig        Do not read from a configuration file, but use the default
                        options instead.
      --strict          When parsing the configuration, check file permissions and
                        ownership and treat warnings as errors.
  -P, --pidfile (file)  Override the pidfile name given in the configuration file with
                        the given file instead.
  -s, --quotes (file)   Override the quotes file given in the configuration file with
                        the given filename instead.
  -j, --journal (file)  Override the journal file given in the configuration file with
                        the given filename instead.
  -4, --ipv4            Only listen on IPv4.
  -6, --ipv6            Only listen on IPv6. (By default the daemon listens on both)
  -t, --tcp             Use TCP. This is the default behavior.
  -u, --udp             Use UDP instead of TCP. (Not fully implemented yet)
  -q, --quiet           Only output error messages. This is the same as using
                        "--journal /dev/null".
  --help                List all options and what they do.
  --version             Print the version and some basic license information.
```

### FAQ
**Q**) How can I use multi-line quotes?  
**A**) Set `QuoteDivider` to `percent` in the configuration file, and delimit each quotation in your quotes file with a percent sign on its own line.

**Q**) Why does this need to run as root?  
**A**) RFC 865 specifies that the quote of the day protocol should be run on port 17, and only root is permitted bind to ports below 1024. By default, the program drops privileges and runs as the `daemon` user once a socket is established.

