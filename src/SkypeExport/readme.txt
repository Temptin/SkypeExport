TO BUILD:
* On Windows, make sure your compiler has the Boost 1.58 (or higher) headers and pre-compiled libraries in the include paths for the compiler and linker (using the included MSVC2013 project is suggested). For ease of use, you can install all pre-built Boost libraries from sourceforge.net/projects/boost/files/boost-binaries/ - the one used to build the current Windows version was the MSVC2013 32-bit installer at http://sourceforge.net/projects/boost/files/boost-binaries/1.58.0/boost_1_58_0-msvc-12.0-32.exe/download (install it to c:\local\boost_1_58_0). If you ever install a newer version of Boost, you must update the library/include paths in the MSVC project. You can use the free Visual Studio Community 2013 to compile this project (https://www.visualstudio.com/products/visual-studio-community-vs); it is meant for open-source projects and uses the exact same high-quality compiler as the paid version of Visual Studio.
* On Mac OS X, look in the _gccbuild folder and read the text document, then issue the build script.
* On Linux, it will be very similar to the OS X build, but possibly needing a different build script (not provided).
* SQLite3 is also required, but is bundled with the project.



*** THIS README FILE IS OLD AND OUTDATED. SEE THE MAIN README.MD INSTEAD. THIS FILE IS JUST PRESERVED HERE FOR THE POSSIBLY-STILL-USEFUL INFORMATION THAT IT CONTAINS ***



WORKS IN BROWSERS: Safari 3/4/5+, Firefox 3/4/5+, Chrome, IE8/9+*. Lower numbers of these don't render the page EXACTLY as it should but just about everything will look right (old browsers COULD be made to work perfectly but I don't want to invest the time in supporting old technology, as doing so would require extra elements/containers on the page and bloated stylesheets to emulate missing CSS features etc). Unlisted browsers not tested. It seems to be that any browser made from about 2008 should support viewing the logs the way they were meant to be viewed. That's almost 4 years, and only utter idiots would run browsers older than that. I'm not going to adapt the page to utter idiots.
Note: * IE8 does render the page nearly perfectly, but is slow as hell and has one minor issue of not supporting the opacity attribute on the icons (so they appear as pitch black). Additionally, the final-output version with embedded images uses data:// uris that are larger than IE8 supports, thereby removing the support for embedded emoticons from IE8. To work around this would require us to split the emoticon sheet into two classes, which is not that hard, but it would also require us to massively split up the world flags sheet and add a bunch of extra CSS, screw that, I'm not going to accomodate the idiotic IE8 (additionally, IE versions below 8 won't even support data:// URIs). Any IE below 9 is utter garbage.
BROKEN IN OLD-AS-HELL BROWSERS: Firefox 2 and earlier renders the page passably but looks quite poor as it lacks CSS3 selectors, extended "display: ..." attributes like inline-* and table-*, and so on. I don't think anyone is dumb enough to still use anything older than at worst Firefox 3 though. IE7 and earlier renders the page in a very messed up manner with the timestamps below the message bodies, as it (just like Firefox 2 and earlier) lacks advanced CSS attributes and selectors; additionally, IE7 and below lack the data:// uri for embedded images which we use in the final stylesheet.



Final Cleanup TODOs [X=done]:
X Use GUID to parse file transfers properly (output a nice list by parsing the transfer data, with all file names and sizes), perhaps even output the local/destination folder, that data is available
X Locate and show the delivery state of messages (pending/delivered)
X Locate and show the state of files (completed/declined or otherwise not transfered) based on GUID lookup
X Get rid of newline chars and replace them entirely with <br /> instead of having both at the same time. this cleans up the XML structure a bit.
X Test multiple files in a single transfer and verify that they're all properly drawn and counted
X Show "This message has been removed." for removed messages
X Safari does not draw the message-bodies correctly (not 100% wide). solved: turns out it simply needed width: 100% on a parent element (yep, that good old width inheritance problem)
X Support smileys by turning all skype smileys into a CSS spritesheet along with a typeID(laugh)->style/smiley map(bigsmile), and finally converting <ss type="laugh">:D</ss> to <div class="ss bigsmile"><span>:D</span></div> UPDATE: I've now also written a converter that renamed the CSS classes to the internal typeIDs, so we don't even have to do any re-mapping; we simply output <ss type="laugh">:D</ss> as <div class="ss laugh"><span>:D</span></div>
X Preserve whitespace, like "<br />     HELLO", which should be an indented line. I think there's a CSS value for that. I could even enable that CSS flag and rely entirely on newlines rather than <br />, but that leads to uglier code since the message bodies will leak out of the markup. Solved: white-space: pre-wrap tells the browser to preserve whitespace sequences, whitespace at the start of lines, as well as parsing \r\n (or any combination thereof) as newlines. However, I'll keep replacing newlines with <br />, for XML cleanliness.
X High-priority cosmetic issue: fix all icons so that they sit bottom-aligned rather than top-aligned: solved by setting "vertical-align: text-top", to align the top of the emoticon element to the top of the text in the parent element, thereby causing downward expansion of the message container, rather than upward, and preserving all padding/formatting the way it should be.
X Improve the icon (not emoticon) sheet by centering the icons and using text-top alignment rather than baseline, for more size-independent handling. Also, ensure that the smaller "edited" state-icon uses baseline alignment instead, so that it always sits perfectly aligned with the baseline of the timestamp.
X Remove trim() around message, since I've found out that message blobs in Skype are actually allowed to begin/end with whitespace. It's extremely uncommon but it happens, so don't trim it.
X Consider adding phone call support. Calls seem to use type:100, blank chatmsg_type, and chatmsg_status 2/4 based on call direction. There's possibly some extra flag that shows whether you/they picked up or not. Additionally, body_xml usually has the value "<partlist alt="">\n</partlist>" when this happens (\n means newline), but not all the time. update: "Calls" seems to have all the data I need to determine if a call was picked up, how long it lasted in seconds, and if it was ignored by me or missed because I was afk ("is_unseen_missed", might also mean the difference between letting it time out or hitting decline). Update: is_unseen_missed is useless as it is only ever 1 when someone has tried calling you for 2 minutes, at which point it times out and sets itself to 0. it doesn't do that if you called someone else. also, call information was limited overall, for instance you cannot see who declined a call request, or if people were afk, and you cannot see who ended an ongoing call, and also the call_start events are at the spot where the call was picked up, not where the call attempt began. finally, there's no correlation between call events and info in the calls database. despite all of this trouble, I managed to correlate enough info to parse calls perfectly.
X See if I have to act on body_is_rawxml. Update: Nope, the value is ALWAYS 1 for the events that matter. The only times it's null is for some instances of events with an empty or null body_xml, so it doesn't matter. When there is content in body_xml, we can be SURE that it's xml-safe.
X (drunk) showed up as (d) aka (drink), due to a stylesheet offset mixup between those two. I took the opportunity to clean that up and to convert the filename-based stylesheet to id-based.
X Added (flag:CC) support.
X Consider: Getting last displayname of someone via Contacts table instead, although the current method is very fast. Update: No, that would be pointless since we STILL have to scan the whole displayname history to build the list of displaynames over time, so we have no use for getting the value directly from the Contacts table, even though the Contacts table DOES contain data even on people you have removed.
X Look into how multiperson chats work/look in Skype and how to parse that; will likely require some changes to "partner's last displayname", etc. Low priority, and pretty easy fix. Also show "X has joined the chat". UPDATE: Conference chats now fully supported; in fact, we support them better than Skype itself, as we will locate all conferences that partner took part in, whereas Skype will only show conferences YOU initiated with THAT particular partner, and nothing else (it's too dumb to log anything else).
X Wide messages without spaces cause the page to become wider (the message element will expand beyond the container width to fit the content). See if I can force wrapping in the middle of words via CSS) (word-wrap: break-word may work now that I have set sizes properly). update: nope, still doesn't work, not even if I try display: block, with various width values, etc. maybe it works if I set a smaller width? update: nope, tried display:block, maxwidth:50%, width:50%, word-wrap:break-word, overflow: hidden, and got the text to break properly but the page was still too wide... hmm... update: found a solution, turns out it's a bug in Firefox, as well as in Safari (it refuses to break utf-8 formatted text), but Safari can be made to behave. I'll implement my fix soon. UPDATE: scrap all the other stuff, I've implemented something the world has never seen before, which perfectly wraps text while allowing the page width to still be dynamic, muhahah nobody else has ever thought of this!
X Handle the mess of Skype's databases mixed \n and \r\n newlines all within the same message; it's truly a wonder how they managed to screw up the protocol like that but I've written some code to accurately replace newline sequences no matter which sequence is used.
X Create an accurate method for looking up conference titles and people's displaynames at certain points in time.
X Create a way to get a list of all conferences a certain person took part in, so that we can output their complete history. Found that the Participants table is the most reliable for this purpose; perfect!
X Devise a way to compress the stylesheet, and convert images to data://, so that it's all embeddable in the html file.
X Finish the file writer so that it writes to the user-specified path, using platform-appropriate newlines.
X Add the person's latest displayname to the page title.
X Fixed issue with the base64-encoded worldflags image in the stylesheet; it was truncated. Solved by writing my own base64 encoder that encodes the data properly.




KNOWN ISSUES, WONTFIX (maybe some day):
* Copying logs and pasting them into formatting-aware programs causes chaos. This has to do with all the containers and their relationships being only partially copied. No solution.
* Copying logs and pasting them as plaintext or pasting into chat windows causes timestamps to appear AFTER the actual message. Possible solution: Move the "T" timestamp element before the message element, and apply loads of CSS to get it to DISPLAY to the right of it. Workaround: Just copy the content of one message at a time; or, if you're sharing the stuff with a friend, give them either a PDF-printout, the HTML file itself, or a screenshot of the relevant portion.
* Copying includes neither the smileys/icons NOR the ":D" text representations. Possible solution: On select-start (onselectstart), swap to a stylesheet that shows these elements. On deselect (onselect fires after selection ends, and we can look at the selection to see if it's blank, then we know it's all been deselected), switch back.
* These copying problems are all solvable with various stylesheet hacks, and I dislike all of them. Feel free to try and submit your patches.
* Hovering over flags doesn't display the country names OR even the 2-letter country code; this is because we're implementing all icons as divs rather than imgs. "Solving" this minor problem would just make things worse, as each image (there are hundreds of smileys, icons, etc) would have to be split into its own, separate file, for the img src attribute. If anyone has a better solution, feel free to provide one.
* Skype inserts <legacyquote> or <quote> blocks for pasted chat quotes, which occur in event type 61 (regular text messages). Add support for them someday... Very low-priority issue, as the contents are properly displayed already.



KNOWN ISSUES, SERIOUS:
* The regular expression library used does not support UTF8/UTF16/UTF32. Requires some major changes to port to the IBM ICU string library and boost:u32regex. The current implementation has a very minor risk of falsely identifying unicode letters as part of a regex when they really shouldn't be matching. That risk will be eliminated once porting has been performed.



FUTURE WISHES:
* Full GUI
* Automatic locating of the Skype folder on Mac, Windows and Linux so that you will only have to provide your username (tricky, as the paths are not fixed, and you may sometimes want to provide just the raw db itself to be parsed)
* Log-picker interface where you can view logs inside the GUI
* Time-range slider based on first and last event (first and last are easy to find), to allow partial export/view
* Stylesheet switcher for other displaystyles, or some sort of color scheme switcher at least (invert, for instance).
* Toggle graphical emoticons on/off, and stuff like that.
* Pure XML output where stuff like <div class="DayContainer" is instead <DayContainer>, for those that would like output that's easily processed via an XML parser
* HTML to PDF without relying on the browser having that feature, as that's mainly available on Macs.


