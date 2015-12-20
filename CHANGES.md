# Release History

## SkypeExport v1.4.0 Stable (December 19th, 2015)

This release brings support for Microsoft's auto-migrated "Windows Live" Skype IDs, and fixes a rare issue with databases containing thousands of conferences. It's the *last-ever* official release of SkypeExport, due to personal time constraints. We already *fully* support the Skype protocol and all of its features in a future-proof way, and we've been stable for 4 years with only a few minor updates when Microsoft bought Skype and added their own Cloud features. *All* past (classic) and present versions of Skype are fully supported - on Windows, OS X and Linux! Have fun and enjoy!

* (Enhancement) Support for exporting Microsoft's auto-migrated "Windows Live Messenger" IDs, which are their internal, auto-created Skype IDs in the format "live:old_messenger_username" (such as "live:rince.farbgold").
* (Bug Fix) Improved conference participant scanner. People with over 1000 conferences were able to reach SQLite's database query limits. The query has now been rewritten and future-proofed to support an unlimited number of conferences.


## SkypeExport v1.3.0 Stable (June 27th, 2015)

This is the final release of SkypeExport, and it contains multiple important fixes to deal with _all_ of the remaining flaws in Skype's unreliable "cloud history" service. Official support for the Linux platform has also been added at last, thanks to a collaborative effort with the brilliant Santosh Thoduka. What a fantastic way to end, with _full_ support for _all_ past and present versions of Skype on Windows, OS X and now _Linux_!

* (Enhancement) Improved the contact name scanner to _ignore_ spammers. We've always used a _very_ thorough method of analyzing the database to be sure that we find and export _everybody_ you've ever talked to, but it had the side-effect of also picking up the names of spammers who sent you friend requests that you'd never accepted. We now ignore those fake friend requests, thus giving you a perfect friends list export.
* (Enhancement) We now _always_ display the "edited" icon next to edited outgoing messages, even if they are still marked as "Pending delivery". This brings us in line with the corresponding GUI changes in Skype 6+, where this new "edited" icon behavior was introduced.
* (Bug Fix) Improved handling of the multiple types of database corruption caused by Skype 6+'s unreliable "cloud history" service: The contact name and display name scanners now _avoid_ yet another type of cloud-induced "empty database field" corruption. The display name scanner now falls back to displaying the raw username if no valid name could be found due to corruption. The message direction scanner has been rewritten for completely reliable event direction detection again, regardless of whether your chat history data is corrupt cloud-data or pristine local history, and the fix is fully backwards-compatible with old databases. The file transfer scanner has been rewritten to deal with the cloud corruption and is able to detect all file transfers in a robust and backwards-compatible manner. We now handle these various cloud service corruptions _much better_ than the official client, just like a _lot_ of _other_ issues with Skype's database that we've had to work around over the years.
* (_Developers_) Added Linux build system graciously contributed by Santosh Thoduka (@sthoduka). To install SkypeExport on Linux, simply download the source code and read the brief instructions in the "gccbuild/linux" directory. Have fun!


## SkypeExport v1.2.0 Stable (June 15th, 2015)

Brings you several great, new time-related features along with a few minor general enhancements.

* (Feature) Added a new "--timefmt" command line switch with options for "12h" (default), "24h", "utc12h" and "utc24h". This allows you to change the format of all history timestamps, as well as optionally switching everything to UTC time instead.
* (Enhancement) Exported histories now show the active UTC offset, such as "All times are UTC+02:00" or "UTC-04:00" or "UTC+09:30", and so on... If you're telling it to output in UTC, or you're in a timezone which is identical to the current UTC time (such as Iceland with DST active), it will say "UTC+00:00". This means that regardless of the output time format, the reader will know exactly what time offsets are being used.
* (Bug Fix) Improved the contact name and display name scanners to avoid yet another extremely rare Skype 6+ bug, wherein it sometimes stores blank names in its database. We now simply skip any blank names in the database and retrieve their nearest valid name instead.
* (Bug Fix) Now handles the rare case of encountering a contact who doesn't have any 1on1 chat. This can happen when you've only seen that person in a conference, such as when a mutual friend added them to a group chat and you've never talked to that person directly.


## SkypeExport v1.1.0 Stable (May 30th, 2015)

This release is recommended for all users, as it gives the emoticons a modern graphical overhaul and adds support for Skype's new cloud-based file transfers.

* (Feature) Switched to the new Skype 7+ emoticons. This gives the project a nice graphical refresh, since their new emoticon set is much better looking and even includes a handful of extra emoticons and world flags.
* (Feature) Now supports Skype's new cloud-based file transfers. This is a new feature introduced in Skype 6.22 and up, where it automatically uploads JPG/PNG images (no other formats for now) into the cloud and then sends a reference to the recipient, instead of doing a regular peer-to-peer transfer.
* (Enhancement) As a result of improvements to the image compression, the permanently embedded stylesheet on all exported history pages is now much smaller! Old CSS: 219001 bytes, New CSS: 172329 bytes (percentage of original: 78%), Savings: 46672 bytes! That's almost 50kb less bloat in each exported history file! Very nice indeed.
* (_Developers_) New spritesheet compiler which completely automates the tedious work of building updated emoticon/worldflag sprites and their associated CSS entries.
* (_Developers_) Automatic image compression script which simplifies deployment of new visual assets (based on the highly efficient ZopfliPNG by Google).
* (_Developers_) Updated Visual Studio project to MSVC2013.


## SkypeExport v1.0.0 Stable (May 16th, 2015)

After 4 years of perfect stability, it's finally time to go out of beta! Version 1.0.0 Stable contains the following changes:

* (Feature) Complete support for the new message [formatting](http://blogs.skype.com/2014/10/16/instant-message-formatting-with-skype-for-mac-7-0/) options added in Skype 7 (**bold**, _italics_, ~~strikethrough~~ and `preformatted`).
* (Feature) Previously, we only had message history notifications for when _you_ started a conference with someone else, but nothing when your _chat partner_ was the one who invited _you_ to a conference. We now display a notification in the message history even when someone else invites you to a conference, which is actually something that even Skype itself cannot do. Having this information available helps make your history easier to understand whenever the context changes from a 1on1 chat to a conference.
* (Bug Fix) New way of handling message history extraction, to solve a rare database bug introduced by Skype 6. It now exports the full message histories again, and the fix is fully backwards-compatible with old databases.
* (Bug Fix) No longer fails when given a Skype database that has never been part of any conferences.
* (_Developers_) Major improvements to the OS X compilation system.
* (_Developers_) New CSS compiler, making it easy to quickly extend the look of the pages.
* (_Developers_) Uses the latest versions of SQLite and Boost.


## SkypeExport v0.9.1 Beta (August 28th, 2011)

SkypeExport was born as a personal project as I was leaving Skype and needed some way of exporting years of conversations, but there were no solutions out there for doing so.

This software was created during an intense 3 week reverse engineering and coding session and was finished on August 28th, 2011, working perfectly with the latest Windows and Mac Skype versions out at that time.

Released as a "Beta" even though it fully supports _all_ features of the Skype protocol, since the software will now have to prove itself out in the world before it will be granted the honor of a 1.0 release.
