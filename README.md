## Curses-based typing tutor, Typist 3.0 (GNU _Gtypist_ fork/clone)

### Why?

In 1998 I wrote the core of what is now GNU's _Gtypist_. At the time I knew little about
modular and O-O programming, and it showed. Since 1998 a lot of good people have made
some excellent changes, additions, and extensions to _Gtypist_ -- and frankly, that is
rather humbling -- yet for two decades I have also felt somewhat embarrassed about the
quality of code I initially delivered.

This finally got the better of me, and so I rewrote it as this. Because I do not actually
have Linux, autoconf and so on to hand, I could not start with the current _Gtypist_ source
base, so instead I began with the original and forward-ported as much of _Gtypist_ into
it as I could in the time available, albeit with a couple of _major_ remaining omissions.

### What is in?

The result is _Typist 3.0_, for want of a better term. Improvements over my distinctly
average initial efforts two decades ago include:
* A fully re-entrant script interpreter, enabling nested script execution
* JSON format lesson data files, for better data portability
* A purely declarative lesson data file format
* Emulation of several keyboard layouts without needing to set them in the OS
* Infinite loop (and infinite recursion) detection
* Better string hashing
* Full error recovery on non-internal (that is, non-coding) errors
* Improved modularity, and better debugging for both lesson scripts and the program itself
* Some bug fixes
* Backwards compatibility with GNU _Gtypist_ lesson files(*)

(*) Except for any that are non-ASCII. Which is actually most of them, to be fair.
Sorry about that.

### What is missing?

Currently (and significantly) missing from _Typist_ but present in GNU _Gtypist_:
* Support for non-ASCII languages
* More flexible colour controls
* Log of past exercises
* A heap of help and other documentation

### What is odd?

I also used this program as a test-bed for a few ideas that I wanted to see how they
might work. Some work better than others.

The attempts at C namespaces are... interesting but could also be seen as more trouble
than they are worth, particularly since they make it possible to successfully link an
incomplete program, so that the missing module only shows up as an attempt to
deference a null pointer when you try to call one of its member functions. Although
part of the aim here was to create the code as it _should have been_ two decades ago,
I should probably have just gone to full C++ for a much easier life.

Also, using JSON as a data file format has both pluses and minuses. _Strict_ JSON is
particularly fiddly and annoying to write with a text editor, although I am not sure
that XML would be much better. The rather excellent
[JSON parser](https://github.com/zserge/jsmn) used by the program has a 'relaxed'
mode that is somewhat simpler, but the tradeoff is that real errors in the input file
are sometimes not properly found or reported. And hiding effectively executable stuff
inside a JSON file, as attempted in the v1 JSON lessons, looks particularly odd (and
hence the v2 and purely declarative JSON lessons). INI file format, maybe, next time?

### Finally, for GNU _Gtypist_ maintainers...

If there is anything at all you wish to extract from this source base and work into
GNU _Gtypist_, you are of course more than welcome to do so.

Some parts will probably be hard to port without considerable surgery to _Gtypist_, but
a few others look like they might be relatively straightforward to fold in. Feel free to
take whatever you can, anyway.
