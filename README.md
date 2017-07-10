# qotd
[![Build Status](https://travis-ci.org/ammongit/qotd.svg?branch=master)](https://travis-ci.org/ammongit/qotd)
[![Coverity Status](https://scan.coverity.com/projects/10274/badge.svg)](https://scan.coverity.com/projects/ammongit-qotd)

QOTD (quote of the day) is specified in [RFC 865](https://tools.ietf.org/html/rfc865) as a way of broadcasting a quote to users. On both TCP and UDP, port 17 is officially reserved for this purpose. This program is meant to provide a simple QOTD daemon on IPv4 and IPv6 over TCP/IP. See also [here](https://en.wikipedia.org/wiki/QOTD).

### Installation from package
* [AUR stable](https://aur.archlinux.org/packages/qotd)
* [AUR development](https://aur.archlinux.org/packages/qotd-git)
* [Fedora](https://admin.fedoraproject.org/pkgdb/package/rpms/qotd/)

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
The default configuration file is located at `/etc/qotd.conf` (though this can be changed with the `-c` flag). Read the man pages or look at the example configuration files for more information about each setting and what it does.

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

