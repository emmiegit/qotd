# qotd
QOTD (quote of the day) is specified in [RFC 865](https://tools.ietf.org/html/rfc865) as a way of broadcasting a quote to users. On both TCP and UDP, port 17 is officially reserved for this purpose. This program is meant to provide a simple QOTD daemon on IPv4 and IPv6 over TCP/IP. See also [here](https://en.wikipedia.org/wiki/QOTD).

### Installation from package
This program is available on the [AUR](https://aur.archlinux.org/packages/qotd-git).

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

### Configuration
The default configuration file is located at `/etc/qotd.conf` (though this can be changed with the `-c` flag). The following options are supported:

<tt>Port <i>positive-integer</i></tt><br>
Specifies an alternate port to listen on. The default is port 17.

**Default:**  use port 17

<tt>PidFile <i>path-to-file</i></tt><br>
Specifies what pid file is to be used by the daemon. If this value is `none`, then no pid file is written. The default is `/run/qotd.pid`, with `/var/run/qotd.pid` as a fallback if `/run` does not exist.

**Default:** `/run/qotd.pid`

<tt>RequirePidFile <i>boolean-value</i></tt><br>
If the process is unable to create a pid file and this setting is true, then the process quits with an error.<br>  
**Default:** yes

<tt>QuotesFile <i>path-to-quotes-file</i></tt><br>
The source of quotations to be displayed. This file is broken down line-by-line, and a random line is chosen to be the quotation. See the `DailyQuotes` option for how this process works.

**Default:** `/usr/share/qotd/quotes.txt` (provided by installation)

<tt>QuoteDivider (line | percent | file)</tt>
How quotes in the quotes file are separated. There are currently three possible options: `line`, `percent`, or `file`.
If the value is `line`, then each non-empty line is treated as a quotation to be possibly transmitted.
If `percent` is set, then the program is instructed to separate each quote with a line that has only a percent sign ('%') on it. More specifically, the program looks for a sequence of newline, percent sign, and newline, and separates the string there.
If `file` is used, then the whole file is treated as one quote.
**Default:** `line`

<tt>DailyQuotes <i>boolean-value</i></tt><br>
This option determines whether quotations will be randomly selected every time a request is made, or if the daemon provides one quote per day.

**Default:** yes

<tt>AllowBigQuotes <i>boolean-value</i></tt><br>
RFC 856 says that quotes should be no bigger than 512 bytes. If this option is set to 'yes',
this limit is ignored. Otherwise, quotes will be truncated to meet the byte limit.

**Default:** no

### Running
_systemd_ users can run the new service:

```
# systemctl enable qotd.service
# systemctl start qotd.service
```

Those running the daemon directly should be aware of its options:

```
Usage:
   qotdd [-f] [-c config-file | -N] [-P pidfile] [-s quotes-file] [-4 | -6]

   qotdd [--help | --version]

Options:
   -f, --foreground
          Do not fork, but run in the foreground.

   -c, --config config-file
          Specify  an  alternate  configuration  file  location. The default is at /etc/qotd.conf.  (Overrides a previous -N option)

   -N, --noconfig
          Do not read from a configuration file, but use the default options instead. (Overrides a previous -c option)

   -P, --pidfile pidfile
          Override the pidfile name given in the configuration file with the given file instead.

   -s, --quotes quotes-file
          Override  the  quotes  file  given  in  the  configuration  file with the given filename instead.

   -4, --ipv4
          Only listen on IPv4.

   -6, --ipv6
          Only listen on IPv6.

   --help
          List all options and what they do.

   --version
          Print the version and some basic license information.
```

### FAQ
**Q**) Why does this need to run as root?

**A**) RFC 865 specifies that the quote of the day protocol should be run on port 17, and only root can bind to ports below 1024. You can set an alternate port in `/etc/qotd.conf` (or whichever configuration file you tell the daemon to use).

