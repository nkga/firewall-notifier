# firewall-notifier

!["Notification sample"](https://raw.githubusercontent.com/nkga/firewall-notifier/master/doc/img/notification.png )

Firewall notification application for Windows 7+, written in C. Installation and dependency free.

### Background

Windows does not natively provide notifications for blocked outbound connections. The default
Windows firewall behavior is configured to implicitly allow all outbound connections.

This program provides notifications for when outbound connections are blocked.

### Usage

1. Ensure `User Account Control` and `Windows Firewall` are enabled.
2. Run the application.

### Notes

- On application startup, all firewall profiles are set to enabled with outbound connection blocking on.
- Manual modification of firewall rules may take a few minutes to propagate to the application's cache.

### Building

1. Install [Visual Studio](https://www.visualstudio.com/).
2. Open `firewall.sln`.
3. Change solution configuration to `Release`.
4. Build solution.
