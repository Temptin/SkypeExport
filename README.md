# SkypeExport

Cross-platform Skype history exporter written in C++. Very fast. Spits out beautiful, completely self-contained HTML pages that don't require any extra files.

Supports all features of the Skype protocol thanks to careful reverse-engineering.

## Compiling

* Windows compilation is done using Visual Studio or the compiler of your choice.
* Mac OS X compilation is done using the included shell script.

## History & The Future

* SkypeExport was born as a personal project as I was leaving Skype and needed some way of exporting years of conversations, but there were no solutions for doing so out there.
* This software was created during an intense 3 week reverse engineering and coding session and was finished on August 28th, 2011, working perfectly with the latest Windows and Mac Skype versions out at that time.
* However, work now needs to be done to see why it's incapable of reading the latest Skype databases as of mid-2013. The SQLite library bundled with this project spits out a read error and my guess is that it might simply need an update.
* I, the author, no longer have any time nor interest in tinkering with this code. It is therefore my hope that someone wants to take over the project.
* The codebase is filled to the brim with extremely detailed comments, making it easy to understand for someone that's determined to read through it all.
* Getting your head around the structure of this project might take a bit longer than a fully-polished, documented project, but everything is actually fairly straight-forward and self explanatory. Just poke around in the folders and read the various files.
* The reason I'm releasing it even in its semi-broken state is simple: It's the best history exporter tool out there. It just needs some tender, loving care and a new maintainer.
* Well, that's it! Have fun!

