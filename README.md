# qotd
QOTD (quote of the day) is specified in [RFC 865](https://tools.ietf.org/html/rfc865) as a way of broadcasting a quote to users. Port 17 on both TCP and UDP is reserved for this purpose. This program is meant to provide a simple QOTD daemon. See also [here](https://en.wikipedia.org/wiki/QOTD).

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
The default configuration file is located at `/etc/qotd.conf` (though this can be changed with the `-c` flag).

The 

### Running
_systemd_ users can run the new service:

```
# systemctl enable qotd.service
# systemctl start qotd.service
```

### FAQ
**Q**) Why does this need to run as root?

**A**) RFC 865 specifies that the quote of the day protocol should be run on port 17, and only root can bind to ports below 1024. You can set an alternate port in `/etc/qotd.conf` (or whichever configuration file you tell the daemon to use).

