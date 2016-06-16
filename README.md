## radial ##

A circular buffer that's totally rad(ial). The idea is based on
[this][tpcircularbuffer], but since I don't use proprietary operating
systems I decided to write one for GNU/Linux. The main trick is creating
an "anonymous file" and then using `mmap(2)` to map it in multiple
addresses.

[tpcircularbuffer]: https://github.com/michaeltyson/TPCircularBuffer

### License ###

This software is licensed under the "LGPLv3 or later" license.

```
radial: a radial buffer implementation that's totally rad.
Copyright (C) 2016 Aleksa Sarai

This program is free software: you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program.  If not, see
<http://www.gnu.org/licenses/>.
```
