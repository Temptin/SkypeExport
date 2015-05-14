# SkypeExport

Cross-platform Skype history exporter written in C++. Very fast. Spits out beautiful, completely self-contained HTML pages that don't require any extra files.

Supports all features of the Skype protocol thanks to careful reverse-engineering.

## Features

* Output all contacts or just a certain subset.
* Edited-state of messages.
* Delivery-state (Pending/delivered).
* Emotes (/me emotes).
* File transfers and their states (including multi-recipient transfers in conferences), their filenames and sizes (in human-friendly format).
* Phone calls (incoming/outgoing/missed/ended).
* Conference chats (their creation as well as their contents and people being added/leaving and all the chat history and events that took place in the conference, along with a custom style to set conferences apart graphically).
* Text messages including dynamic page-width based wrapping of extremely long words.
* Text with multiple whitespaces and/or indentations is drawn properly.
* Removed messages ("This message has been removed.").
* Sender of a message (with their exact name at that point in time).
* Web links.
* Timestamps.
* Emoticons (all of them).
* All country flag icons.
* Detection and output of Display Name changes for yourself and others without needing a change in message-chunk direction (not even Skype does this!).
* Complete history export including every conference they were ever added to/talked in (this is something that even Skype itself cannot do!).
* The entire history for a chat contact is output as a *single* XHTML file that works in all modern browsers and contains the *entire* history and *all* images, stylesheets, etc (in a highly compressed format); this means you can share a single .htm file and it will contain the entire history and everything required to render it properly!
* It even outputs the complete history for people you have long-since deleted from your contact list!
* ...and much more (actually, that's all the major stuff, so not that much more ;)).

## Compiling

* Windows compilation is done using Visual Studio or the compiler of your choice.
* Mac OS X compilation is done using the included shell script.

## History & The Future

* SkypeExport was born as a personal project as I was leaving Skype and needed some way of exporting years of conversations, but there were no solutions out there for doing so.
* This software was created during an intense 3 week reverse engineering and coding session and was finished on August 28th, 2011, working perfectly with the latest Windows and Mac Skype versions out at that time.
* As of May 2015, it still works perfectly with all of the latest versions of Skype; verified using Skype 7 on OS X.
* I, the author, have very little time for tinkering with the code since it already fully supports the Skype protocol and is extremely robust. What remains to be done is just minor user-experience work such as offering an easier interface. If you are interested in ways to help make the project even better, the most important one would be to create a graphical user interface for this command line application.
* The codebase is filled to the brim with extremely detailed comments, making it easy to understand for someone that's determined to read through it all.
* Getting your head around the structure of this project might take a bit longer than a fully-polished, documented project, but everything is actually fairly straight-forward and self explanatory. Just poke around in the folders and read the various files.
* Well, that's all for now! Have fun!

