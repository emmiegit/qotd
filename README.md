# qotd
QOTD (quote of the day) is specified in [RFC 865](https://tools.ietf.org/html/rfc865) as a way of broadcasting a quote to users. On both TCP and UDP, port 17 is officially reserved for this purpose. This program is meant to provide a simple QOTD daemon. See also [here](https://en.wikipedia.org/wiki/QOTD).

### Installation from package
Currently no distros have this project in any of their repositories. If you create a package, please tell me so I can update this page.

### Installation from source
Clone this repo, and in the top level directory, run:

```
$ make
# make install
```

This will install the following files on your system:

* `/etc/qotd.conf`
* `/usr/bin/qotdd`
* `/usr/lib/systemd/system/qotd.service`
* `/usr/share/qotd/quotes.txt`

If you don't use _systemd_, install with the following instead:

```
# make install-no-systemd
```

### Configuration
The default configuration file is located at `/etc/qotd.conf` (though this can be changed with the `-c` flag). The following options are supported:

<tt>Port <i>positive integer</i></tt>
Designates an alternate port to listen on.
**Default:**  use port 17

<tt>PidFile <i>path to file</i></tt>
Designates what pid file to be used by the daemon. If this value is `none`, then no pid file is used.
**Default:** `/var/run/qotd.pid`

<tt>RequirePidFile <i>boolean value</i></tt>
If the process is unable to create a pid file and this setting is true, then the process quits with an error.
**Default:** yes

<tt>QuotesFile <i>path to quotes file</i></tt>
The source of quotations to be displayed. This file is broken down line-by-line, and a random line is chosen to be the quotation. See the `DailyQuotes` option for how this process works.
**Default:** `/usr/share/qotd/quotes.txt` (provided by installation)

<tt>DailyQuotes <i>boolean value</i></tt>
This option determines whether quotations will be randomly selected every time a request is made, or if the daemon provides one quote per day.
**Default:** yes

<tt>AllowBigQuotes <i>boolean value</i></tt>
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


### FAQ
**Q**) Why does this need to run as root?

**A**) RFC 865 specifies that the quote of the day protocol should be run on port 17, and only root can bind to ports below 1024. You can set an alternate port in `/etc/qotd.conf` (or whichever configuration file you tell the daemon to use).

