## radial ##

A circular buffer that's totally rad(ial). The idea is based on
[this][tpcircularbuffer], but since I don't use proprietary operating
systems I decided to write one for GNU/Linux. The main trick is creating
an "anonymous file" and then using `mmap(2)` to map it in multiple
addresses.

[tpcircularbuffer]: https://github.com/michaeltyson/TPCircularBuffer
