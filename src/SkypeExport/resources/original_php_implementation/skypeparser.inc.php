<?php

// WARNING: THIS FILE IS ONLY HERE FOR HISTORICAL REASONS. IT WAS THE FIRST PROTOTYPE AND REVERSE ENGINEERING ATTEMPT AND WAS JUST USED TO AID RAPID DEVELOPMENT, WITHOUT HAVING TO CONSTANTLY RECOMPILE TO TEST CHANGES. IT IS HOWEVER EXTREMELY SLOW AND VERY FAR FROM COMPLETE, AS ALL DEVELOPMENT EFFORTS MOVED OVER TO THE C++ VERSION ONCE THE PHP PROTOTYPING WAS DONE. IN OTHER WORDS: DO WHATEVER YOU WANT WITH THE PHP IMPLEMENTATION, BUT DON'T EXPECT IT TO BE PERFECT (SINCE THE PROTOCOL HAD NOT BEEN FULLY FIGURED OUT HERE).

	set_time_limit(0); // the default expiration time of 30 seconds is fine for just about every log; even a log with 40k entries takes about 18 seconds, but if you parse multiple logs in a row that's where this comes in.
	date_default_timezone_set('Europe/Stockholm'); // makes it so that all timestamp->timestring conversions will be done in relation to Sweden's local time
	
	/* SkypeParser Class */
	class SkypeParser
	{
		private $dbPath = "";
		private $db = NULL;
		
		public function __construct($sDBPath)
		{
			// load DB
			$this->dbPath = $sDBPath;
			$this->db = new SQLite3($this->dbPath, SQLITE3_OPEN_READONLY);
		}
		
		/*
			Generates a list of encountered skype users and their display names over time.
		*/
		public function getSkypers()
		{
			$aSkypers = array();
			$sQuery = "SELECT DISTINCT author, from_dispname FROM Messages ORDER BY timestamp ASC";
			$result = $this->db->query($sQuery);
			while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
				if (!array_key_exists($row["author"], $aSkypers)) $aSkypers[$row["author"]] = array($row["from_dispname"]); // first nick for user
				else $aSkypers[$row["author"]][] = $row["from_dispname"]; // newer nick
			}
			return $aSkypers;
		}
		
		/*
			Grabs the latest displayname for a given Skype ID. A much faster alternative to getSkypers().
		*/
		public function getLastDispName($sSkypeID)
		{
			$sQuery = "SELECT from_dispname FROM Messages WHERE author='".$this->db->escapeString($sSkypeID)."' ORDER BY timestamp DESC LIMIT 1";
			$result = $this->db->query($sQuery);
			while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
				return $row["from_dispname"]; // returns the displayname
			}
			return false; // no such skypeID
		}
		
		/*
			Gives you the title of a conference. It's as simple as that.
		*/
		public function getConferenceName($iConvoID)
		{
			// grab the name of the conference (this is actually the final name, as it turned out after everyone had been added, even the people that were added later than the initial creation)
			$confquery = $this->db->prepare("SELECT displayname FROM Conversations WHERE id=?");
			$confquery->bindParam(1, $iConvoID, SQLITE3_INTEGER);
			$confinforesult = $confquery->execute();
			
			// since the above query will only ever return 1 result, we can simply grab that row without any loop
			/*
				displayname (string): the name of the conference minus yourself. some names may sometimes be represented as "..." (might happen for people that aren't on your contact list, not sure how Skype determines it, or maybe it happens for conferences you did not create)
			*/
			$confinfo = $confinforesult->fetchArray(SQLITE3_ASSOC);
			
			// return the name // FIXME: in C++ we may want to just turn Conversations into a built-once map so that we can always look up convo ids during log parsing and during conference output
			return $confinfo["displayname"]; // fixme: no error checking
		}
		
		/*
			Outputs the complete conversation history with the given Skyper (if arg1 is a SkypeID and arg2 is false) or a given conference chat (if arg1 is a convo_id and arg2 is true).
		*/
		public function showHistory($searchValue, $bIsConference)
		{
			// Preparation of name lookup storage
			$_chatParticipants = array(); $_lastDialogPartnerSkypeID = ""; // chatParticipants contains skypeID -> "most-up-to-date displayname at that point in time" mappings which are constantly being updated as the name changes during the course of the log parsing; for 1on1 it's autofilled with the first appropriate displayname for your partner and then updated over time; for conferences it's slowly filled as you encounter new skypeIDs (we can use this delayed method for conferences since we do not care about incoming displaynames most of the time). lastDialogPartnerSkypeID contains, during 1on1: a static SkypeID of the person you were talking to (this can always be relied on to correspond to your partner's id). during conferences: it contains the SkypeID of the last person to send an incoming event of a supported type, allowing us to use it to detect more than 1 chat partner and output appropriate containers for each when the sender changes.
			/*
				DisplayName Lookup Rules:
					Sender:
						INCOMING: always use $row["from_dispname"] to get the name of the sender.
						OUTGOING: always use "You" or similar "Sending X to" grammar. for cases where your name should be part of the message (such as /me strings), use $row["from_dispname"].
					
					Target:
						WHERE TARGET IS A CONFERENCE: always use "to the conference".
						FOR EVENTS THAT REQUIRE "identities" PARSING: Use $_chatParticipants (FIXME: implement a function in C++ that gets the name for a person at a specific point in time).
						FOR REGULAR CHATS: use $_chatParticipants[$_lastDialogPartnerSkypeID] to get the name of your chat partner.
			*/
			if (!$bIsConference) { // if this is 1on1, we must store the $searchValue (their SkypeID) as the static partner ID, and then grab the person's earliest-known displayname from the map of names->displaynamehistory and put it in our chatParticipants name lookup storage once. the reason we must do this is in case we come across something like "you sent a file to X" as the first event we have with that person, in which case we NEED their earliest name so we know what to output as their displayname for that event.
				$_lastDialogPartnerSkypeID = $searchValue; // will be static for 1on1 chats, since no other code modifies the value during 1on1 runtime
				
				// grab their earliest displayname, as that will perfectly correspond with their name at the start of our history with the person FIXME: DELETE THIS PHP SOLUTION DURING PORTING. in C++, simply call the "getDisplayName" on them with the request to get the EARLIEST displayName.
				$namelookup = $this->db->query("SELECT from_dispname FROM Messages WHERE author='".$this->db->escapeString($_lastDialogPartnerSkypeID)."' ORDER BY timestamp ASC LIMIT 1");
				$nameresult = $namelookup->fetchArray(SQLITE3_ASSOC);
				$_chatParticipants[$_lastDialogPartnerSkypeID] = $nameresult["from_dispname"]; // store the first-found name for that person
				
				// fixme: if we ever support partial history export (via history ranges), then this will need to be modified a bit to grab their name at that point in time, using the C++ function to grab someone's name at a specific point in time.
			}
			
			
			// Header NOTE: I first tried to do this with ternary but there are conditions in so many places that it became way too unmaintainable. The repetition cannot be helped.
			if (!$bIsConference) { // regular chat (searchValue = their skypeID)
				echo	"<div class=\"ChatHistory\">".PHP_EOL . // starts the main log-container for this person
						"	<div class=\"HistoryHeader\">".PHP_EOL . // starts the history-header which has some brief info about the generated log
						"		<div class=\"SkypeLogo\"></div>".PHP_EOL . // the Skype logo
						"		<div class=\"LogHeaderLine1\">Chat History with <a href=\"skype:${searchValue}\">".$this->getLastDispName($searchValue)." (${searchValue})</a></div>".PHP_EOL . // clickable name for the person whose history you're exporting (always use the absolute latest displayname for the header)
						"		<div class=\"LogHeaderLine2\">Created on ".date("F jS, Y \\a\\t g:i:s A\\.")."</div>".PHP_EOL . // the log creation date
						"	</div>".PHP_EOL; // closes the history-header
			} else { // conference (searchValue = convo_id of the conference)
				echo	"<div id=\"conf_".$searchValue."\" class=\"ChatHistory ConferenceHistory\">".PHP_EOL . // starts the main log-container for this conference (contains anchor-ID)
						"	<div class=\"HistoryHeader\">".PHP_EOL . // starts the history-header which has some brief info about the generated log
						"		<div class=\"SkypeLogo\"></div>".PHP_EOL . // the Skype Conference logo
						"		<div class=\"LogHeaderLine1\">Conference History for \"".$this->getConferenceName($searchValue)."\"</div>".PHP_EOL . // title of the conference room, grabbed from the database
						"		<div class=\"LogHeaderLine2\">Created on ".date("F jS, Y \\a\\t g:i:s A\\.")."</div>".PHP_EOL . // the log creation date
						"	</div>".PHP_EOL; // closes the history-header
			}
			
			
			// Start!
			$fStart = microtime(true);
			
			
			// Grab Messages
			$sQuery =	"SELECT type, chatmsg_type, chatmsg_status, author, from_dispname, body_xml, timestamp, edited_timestamp, dialog_partner, guid, call_guid, convo_id, identities FROM Messages WHERE " .
							(!$bIsConference ?
								"dialog_partner='".$this->db->escapeString($searchValue)."'" : // grabs all messages and status nodes to/from the person with the given skypeid.
								"convo_id='".$searchValue."'") . // grabs all messages and status nodes to/from the given conference chat.
						" ORDER BY timestamp ASC";
			$result = $this->db->query($sQuery);
			
			
			// Output Result
			$_activeDay = ""; $_chunkDirection = ""; $_chunkOpen = false; $_callDetails = array(); // will hold the active day, current chunk direction, whether the current chunk tag is still open or has been closed, and the call details array
			while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
				/*
					type (integer): exact type of the message, more reliable than trying to understand chatmsg_type+chatmsg_status pairs, since this identifies events clearly, but use in conjunction with the latter.
					
					chatmsg_type (integer): the type of event. slightly different from the more globally reliable "type" above.
					
					chatmsg_status (integer): the transfer status of the event.
						1: pending, outgoing (events that have not yet been delivered to the recipient, either because they are offline or because you lost your P2P connection)
						2: delivered, outgoing (events that have been delivered to the recipient)
						4: delivered, incoming (events that have been delivered to you)
						* no other values will occur
					
					* type/chatmsg_type/chatmsg_status meanings:
						* WARNING: DO NOT USE chatmsg_status to determine direction during parsing; use newEventDirection which will be I for incoming or O for outgoing.
						type:61=regular text message
							chatmsg_type:3 (always)
								chatmsg_status:1=outgoing (pending), 2=outgoing (delivered), 4=incoming (delivered)
						type:60=/me emote
							chatmsg_type:7 (always)
								chatmsg_status:2=outgoing (delivered), 4=incoming (delivered)
						type:68=file transfer
							chatmsg_type:7=file transfer, I want to be able to say that this is ALWAYS used but there was ONE exception below.
								chatmsg_status:2=outgoing, 4=incoming
							chatmsg_type:18=? fixme: only seen this ONCE, in my entire Skype history, and it was indeed a file transfer, of an image, making this "18" value very strange.
								chatmsg_status:2=outgoing, 4=incoming
							* my guess is we should rely on type:68 and just ignore chatmsg_type for file transfers. chatmsg_status still shows the actual direction of the transfer.
							* use guid to look up the transfer details
						type:30=call start (same _type and _status as 39)
						type:39=call end
							chatmsg_type:18 (always)
								chatmsg_status:2=outgoing, 4=incoming (NOTE: ONLY USABLE FOR CALL START; call_end events haven't got the proper direction status to show who really ended the call and should NOT be used for that)
							* the call_start event happens either A) when they pick up, or B) when the program gives up trying to call them (assumes they're not going to answer, which takes 2 minutes of trying (in Skype 5 as of now, may change later))
							* the call_end happens at exactly exactly the same timestamp as call_start when they declined/didn't pick up (I have never seen it differ even 1 second), otherwise the timestamp difference compared to call_start will be the call duration
							* use call_guid to identify each call. WARNING: call_guid is null for VERY OLD calls performed in Skype versions older than 5.
							* more data can be found in the Calls database, but it's nothing we can't determine from just these 2 events. that db has stuff like whether they MISSED the call by not responding for 2 minutes, and any microphone problems etc. useless info.
							* Skype's OWN history viewer does the same thing; it shows the start and end events at the exact same point in time in case it was declined/no pickup. we can even improve that, by just saying "call declined". sadly we have no way to look up the time that the call attempt was REALLY STARTED. that data DOES seem to be available in the Calls database, which contains a begin_timestamp that seems to show the time you BEGAN the call attempt. sadly, that database lacks a call_guid column so it's very difficult to correlate the data with the start/end events. I am guessing that is why even Skype's coders opted to ignore the data in the Calls db and just show start/end at the exact same time for missed/declined calls. the Calls db contains NOTHING to correlate with the Messages db, and it doesn't even contain info about the participant; it contains a binary field with the BINARY GUIDs of the people in the call. the only good thing about it is that it has a conv_dbid column that correlates to the conversation ID, which seems to be a constant ID for each partner pairing, so the ONLY thing you could do is to look at the conversation ID while scanning Messages, and when you see a new ID you read in all call data for that conversation ID, and then you begin parsing messages. you must then look at the previous AND current Message timestamp to determine if you should throw in a "call attempt started" message line there. it's all too much work and too slow, and I bet that is why Skype themselves don't do it.
						type:10=person added to group conference; if you were the one that created the conference then the dialog_partner will be the person you were chatting with when you pressed the "add to conference" button, and the convo_id will be the conference ID. if someone ELSE adds you, the convo_id is 0 (except possibly re-used IDs on re-used conferences) and dialog_partner is null. "participant_count" is the number of people in the conference (this number will keep rising as more people are added, and lowering as people leave). when you first join a conference CREATED BY SOMEONE ELSE, an event type "100" is also fired, with an identities field like "coolguy123 otherguy123 grince.farbgold", in other words a space-separated list of all current participants (any people that you don't have on your contact list is added to your Contacts database, with is_permanent=0 to signal that you have not added them to your permanent contacts, but that their data is available for use i.e. getting their displaynames). after that, it's up to you to keep track of joins/leaves/participant counts via events types 13 and 10.
							chatmsg_type:1 (always, apparently)
								chatmsg_status:as usual this means the direction (incoming/outgoing) of the event
									* parsing strategy: parse history for dialog_partner, and at the first sign of type:10 you grab the new convo_id and store it in an array as "conferences started by us", then look up the associated displayname (in the Conversations database) for that conference, and output a chat link saying "from_dispname (our name) started a conference: name of conference". later, after you have parsed the log, you grab any missing conference ids and then finally loop through your "convo_id" (conference) arrays and output those too at the end of the document, with appropriate "<div id=conf_74327>" tags to allow the above conference links to work.
									* also note: the initial event that started the conference is ONLY logged for the person you were chatting with at the time and ONLY IF *YOU* CREATED THE CONFERENCE, NOT for any of the people that were added to the conference, and NOT if THEY invited YOU, so you will not see conference links for those people when viewing their logs; you only ever see links when YOU created the conference, and ONLY for the one person you were chatting with at the time you pressed the "+add people" button.
									* we will not be parsing event 100 (the "people in the conference" sync event), because we don't care who is already in the conference.
									* the "identities" field for event type 10 is USUALLY *one* name because of how people use it (when YOU *and other people* become invited to a conference, meaning when some third party selects you and a bunch of others ot make a conference, then you will get a type 10 with JUST your name in it, and a type 100 with the other names. also, when someone adds JUST ONE person to the conference, you get 1 identity. the trouble happens when someone adds 2+ people after the conference started, OR when *YOU* start the conference with multiple people selected. in those cases, the identities field will have as many space-separated SkypeIDs as needed.
						type:13=person left the conference.
							chatmsg_type:4 (always, apparently)
								chatmsg_status:as usual this means the direction (incoming/outgoing) of the event; if it is incoming then someone else left, and if it is outgoing then you left.
									* this one is really easy to handle, just use the author/from_dispname to show the person that left for incoming, or write "you left" for outgoing. unlike type 10, "identities" is not used at all here, thankfully.
									* this event only occurs inside conference convo_id streams, so no need to check the isConference flag.
					* note: outgoing means from YOU to the partner --->, and incoming means from the PARTNER to you <---
					
					author (text): the SkypeID of the person that wrote the message (such as andy.norin). will NEVER be null.
					
					from_dispname (text): the active displayname of the person that wrote the message (such as Anders Norin). will NEVER be null.
					
					body_xml (text): the text that was written. it's fully xml-compliant, so stuff like apostrophes are &apos; (well, there IS a flag called body_is_rawxml which is either 1 or null, but I had a look and the only time it has been null is the times when body_xml has been either empty or null itself, so we can safely assume that body_xml is ALWAYS encoded as safe xml)
					
					timestamp (integer): the unix timestamp of when the message was first sent (not when it was delivered, which may come later)
					
					edited_timestamp (integer): null most of the time, but when it contains a value it gives the time of the last edit to the message
					
					dialog_partner (text): the name of the person you are chatting with in single-person chats. always has the same value even when THEY send YOU a message. example: "andy.norin". this value is null in multiperson chats (conferences), apart from the very first message in a conference where it seems to denote the person you were chatting with when the conference was created.
					
					guid (blob): this is a globally unique identifier for the message. we only need it when parsing filetransfer data, as the filetransfer information is stored under this guid.
					
					call_guid (text): this is the call globally unique identifier, used for identifying individual calls' start and end events. only contains any data for event types 30+39. WARNING: call_guid is null for VERY OLD calls performed in Skype versions older than 5.
					
					convo_id (integer): this is the unique identifier for your dialog partner combination; it is permanent (the same whenever you talk to the same partner). also, whenever you create a conference chat (more than 2 people), it generates a new convo_id. The "conversations" database contains mappings for id -> partner's "identity" (skypeID) and other info. we'll mainly be using convo_id to correlate calls in the Calls database. LONG NOTE: However, you should NOT use convo_id to "locate all events between you and that person"; use dialog_partner for that, because otherwise you will be lacking some events, primarily (or only? it is the only missing one I have observed) event type 10 aka the creation of a new group conference chat (by inviting more chat partners), as that event will still have a dialog_partner that's the same as the person you were chatting with, but the convo_id will be new (the new convo id for that new group conference, to be precise) - note that this is useful info if you want to parse group convos too, simply store all eventtype 10 convo ids and then parse those too; the first event of a group conference has the dialog_partner you were with when the group conference was started, and the remaining ones have a null value for dialog_partner.
					
					identities (text): this is NULL for EVERY event type EXCEPT a handful. when it's NOT null, it is a space-separated list of one or more skypeIDs of the people that are taking part of that event. here are the eventtypes where it is used: 110 (unknown event). 100: conference sync event (contains list of existing conference participants, seems to only be sent out when you are added to someone else's conference). 51 (unknown event, almost 100% sure it is for accepted friend request). 50 (friend request; status 2/4 indicates direction). 39 (call start, contains list of people called; one in 1on1, one or more in conference). 30 (call end; value similar to call start, except it may possibly contain early dropouts as single identities entries, until finally everyone has dropped out; OR perhaps it just indicates when YOU hung up and the people that were also in that call, even people that dropped out earlier? FIXME: test that? or not give a shit? I prefer NOT GIVE A SHIT!). 10 (conference join event; identities contains the list of people that were added, usually just one but will be several if more than one person is added at a time). oddly enough event 13 (conference leave event) does NOT use identities, instead the "author"/"from_dispname" is the person that left (which can be yourself, just use chatmsg_status to judge direction to see if it was you that left)
				*/
				switch ($row["type"])
				{
					case "61": // regular text message
					case "60": // /me emotes
					case "68": // file transfer
					case "30": // call start
					case "39": // call end
					case "10": // person(s) added to conference
					case "13": // person left the conference
						// Grab the original and (optional) edited timestamp of the event
						$aTime = array(
							"original" => $this->skypeTimeToString($row["timestamp"]),
							"edited" => ($row["edited_timestamp"] != NULL ? $this->skypeTimeToString($row["edited_timestamp"]) : false)
						);
						
						
						// Generate a textual timestamp for the message, always showing the original send-time, with an "Edited" icon prepended if the message has been edited.
						$sDisplayTime = ($aTime["edited"] !== false ? "<div class=\"icons msg_edit\"><span>Edited</span></div> " : "").$aTime["original"][1];
						
						
						// If the day of this event differs from our current section, create a new day-section.
						if ($aTime["original"][0] != $_activeDay) {
							if ($_chunkOpen) { echo "			</div>".PHP_EOL."		</div>".PHP_EOL; $_chunkOpen = false; } // close the previous <div class="MC"> message-chunk container if one is open
							$_chunkDirection = ""; // reset the chunk direction to tell the chunk-output code below that we need to start a new message chunk (since we closed the current one due to end of day)
							if ($_activeDay != "") echo "	</div>".PHP_EOL; // close the previous <div class="DayContainer">, unless _activeDay is "" (meaning this day is the first one output for the current log)
							$_activeDay = $aTime["original"][0]; // store the active day to prepare for detecting the next day
							echo "	<div class=\"DayHeader\">".$_activeDay."</div>".PHP_EOL; // output day-marker
							echo "	<div class=\"DayContainer\">".PHP_EOL; // begin a message container for this day
						}
						
						
						// Grab the chunk direction (1: pending, outgoing. 2: delivered, outgoing. 4: delivered, incoming)
						$thisEventDirection = ($row["chatmsg_status"] == "1" || $row["chatmsg_status"] == "2" ? "O" : "I"); // O = outgoing, I = incoming
						
						
						// Determine whether we should make a new chat message container. Reasons for doing so: Day changed (day changes close the chunk and set $_chunkDirection to "", which is how it knows to make a new container for the new day). Person talking changed (during conference). Message direction changed (incoming/outgoing). Someone (even yourself) changed their displayname (either in 1 on 1 or conference).
						$bMakeNewContainer = false;
						if (!array_key_exists($row["author"], $_chatParticipants) || $_chatParticipants[$row["author"]] != $row["from_dispname"]) { // if this is the first time we've encountered the displayname, or if the displayname has changed since last time this person spoke, output a new block; this catches newly discovered people and name changes of existing people (including ourselves), even mid-stream while they're talking. NOTE: since this whole code block only triggers for events we actually care about outputting (thanks to the switch above), we are SURE that the event that we're working on will actually be output by us graphically, so it's safe to assume that if the name changed we should output a new messagecontainer for that person (them OR us) as there WILL be AT LEAST 1 event to put in it.
							$bMakeNewContainer = true;
							$_chatParticipants[$row["author"]] = $row["from_dispname"]; // this works because author and from_dispname are never null and always correlate. it also gives us the advantage that "chatParticipants" will always contain the most up-to-date display name for someone at that point in time, meaning that we always know their exact name without resorting to trickery.
						}
						if ($bIsConference && $thisEventDirection == "I" && $row["author"] != $_lastDialogPartnerSkypeID) { // CONFERENCES ONLY: if the INCOMING event author (author=SkypeID) changed (even though the direction MAY not have switched over from incoming to outgoing yet), then we'll also need a new block; this catches multiple incoming participants in a row in a conference chat. we never perform this lookup during regular 1on1 chats, as we've already initialized it to a static skypeid value if it's a 1on1 chat.
							$bMakeNewContainer = true;
							$_lastDialogPartnerSkypeID = $row["author"]; // 1on1: already contains the static skypeid for the person you are talking to. conference: contains the last INCOMING skypeid, to allow us to notice multiple people.
						}
						if ($thisEventDirection != $_chunkDirection) { $bMakeNewContainer = true; } // if the direction changed, we need a new block; this catches direction transitions between incoming/outgoing events. note: we'll take care of actually updating $_chunkDirection to match during the chunk output below 
						
						// Check whether we should create a new message-chunk container to indicate whoever is speaking/sending the event
						if ($bMakeNewContainer) { // change of message direction, or a new day has occured and reset the _chunkDirection variable to "", or the dialog partner name has changed while we're still receiving incoming messages (happens in 1on1 if the partner changes name without you sending them any messages inbetween; and in conference chats when different incoming people write to you all in one "incoming" stream without you saying anything)
							if ($_chunkOpen) { echo "			</div>".PHP_EOL."		</div>".PHP_EOL; $_chunkOpen = false; } // close the previous <div class="MC"> message-chunk container if one is open
							
							$_chunkDirection = $thisEventDirection; // store the active chunk direction
							echo	"		<div class=\"MC S".$_chunkDirection."\">".PHP_EOL . // begin the new message chunk, with the current direction (status) as an appended class
									"			<div class=\"A\">".$row["from_dispname"]."</div>".PHP_EOL . // author of the message-chunk
									"			<div class=\"C\">".PHP_EOL; // container for holding all the messages in this message-chunk
							
							$_chunkOpen = true; // we've opened a new chunk
						}
						
						
						// if this is a "call start" event, look up the call details and store them for later perusal.
						if ($row["type"] == 30) {
							// try to look up the Call information by looking up a matching entry in the Calls database
							// call information exists even for the oldest, pre-Skype 5 calls
							// however, no direct mapping exists, so we'll have to sort calls in reverse by id, where the convo_id is the same, and the timestamp is <= the timestamp of the "call start" event.
							$callinfoquery = $this->db->prepare("SELECT duration, is_incoming FROM Calls WHERE (begin_timestamp<=? AND conv_dbid=?) ORDER BY id DESC LIMIT 1");
							$callinfoquery->bindParam(1, $row["timestamp"], SQLITE3_INTEGER);
							$callinfoquery->bindParam(2, $row["convo_id"], SQLITE3_INTEGER);
							$callinforesult = $callinfoquery->execute();
							
							// since the above query will only ever return 1 result, we can simply grab that row without any loop
							/*
								(NOT USED) begin_timestamp (integer): the true time that the call event was received; this is the actual first "ring" event. in 2 minutes of ringing with no pickup (in Skype 5 as of now, may change later), the regular Messages log will be generating a 30 and 39 event with identical timestamps to say "call of duration 0", which is a very stupid way of logging things. they should have stored the call start at the moment it really happened, and also had call_guid correlate to the Calls database for fuck sake... well, anyway...
								
								(NOT USED) is_unseen_missed (integer): THIS ONLY RELATES TO *INCOMING* CALLS *TO YOU*. this is 1 you don't pick up before the call attempt TIMES OUT (2 minutes or so is the timeout), otherwise 0. note that this value is ONLY for actual TIMEOUTS (2 minutes of them trying to call you and you not picking up at all), NOT when the caller (or the callee) aborts the attempt manually!
								
								duration (integer): the call duration in seconds if picked up, otherwise null
								
								is_incoming (integer): 1 if true 0 if false
								
								(NOT USED) is_conference (integer): 1 if true, null otherwise
							*/
							$callinfo = $callinforesult->fetchArray(SQLITE3_ASSOC);
							
							// store the resulting information in the call details array
							if ($callinfo !== FALSE) { // verifies that a result was indeed returned
								$_callDetails[$row["convo_id"]] = array( // only 1 call is legal at a time for any particular convo-id; if you call someone, both of your "call" buttons become "hang up" instead, even while the call is pending, and even if you both try calling each other at exactly the same time, so there is no way to have multiple calls at once with the same person (FIXME: not tested with conference calls, where some people may be able to drop out and call again later to re-join?, but I don't care about that, and I doubt it makes any difference to the actual Calls database information).
									"duration" => $callinfo["duration"],
									"is_incoming" => $callinfo["is_incoming"],
								);
							}
						}
						
						
						// Format the message for display
						$sMessageBody = "";
						switch ($row["type"])
						{
							case "61": // regular text message
								if ($row["body_xml"] == "") $sMessageBody = "<div class=\"t61_r\">This message has been removed.</div>"; // an empty message means that the sender has hit "remove"
								else $sMessageBody = $this->skypeMessageToXHTML($row["body_xml"]);
								break;
							case "60": // /me emotes
								$sMessageBody = $row["from_dispname"]." ".$this->skypeMessageToXHTML($row["body_xml"]);
								break;
							case "68": // file transfer
								// look up the files involved in the transfer via the Transfers table, going by GUID
								$binaryquery = $this->db->prepare("SELECT filename, filesize, filepath, partner_dispname, status FROM Transfers WHERE chatmsg_guid=:guid ORDER BY id ASC");
								$binaryquery->bindParam(":guid", $row["guid"], SQLITE3_BLOB);
								$transferinfo = $binaryquery->execute();
								
								// generate file-lines
								$iTotalSize = 0; $iTotalFiles = 0; // counts the total size of all files for this transfer, and the total number of files
								$aConferenceOutgoingFiles = array(); // will hold info about each unique outgoing file during a conference
								$aFileLines = array(); // will hold each individual file-line, and will therefore also let us count the number of files involved in the transfer
								while ($fileinfo = $transferinfo->fetchArray(SQLITE3_ASSOC)) {
									/*
										filename (text): the plain name of the file, such as "the.phantom.menace.1080p.mkv"
										
										filesize (text, even though you would expect integer, but I guess they did this to avoid having too-small integers to hold the data in-memory?, luckily we can query it as int to have SQLite cast it for us): the size of the file in bytes, such as "2444978"
										
										filepath (text): the full path to where we saved the file (if receiving) or where it came from (if sending). this contains stuff like "/Volumes/Secondary OS/path/to/file.jpg". it is null if it was an incoming transfer that WE never accepted (either because they cancelled it or because we cancelled it), since Skype has no idea where it WOULD have been saved if we'd have accepted. we'll ONLY use this value during SENDING, and only when that's done TO CONFERENCES. when that's the case, we will use it to avoid counting the same file multiple times for each participant. (we will NEVER use this value for receiving, since receiving files in a conference only inserts ONE entry per file in the Transfers database, we don't know the state of hte others in the conference and don't need to bother with that; as for outgoing transfers to 1on1 chats, we likewise only have 1 entry per file since there's only 1 recipient; we ONLY need it for OUTGOING TO CONFERENCE). note: I wish we could have just used the participant_count column in the Messages table, as that would have been such a clean solution, but sadly that value is null for pre-Skype 5 events...
										
										partner_dispname (text): the displayname of your chat partner. for 1on1 chats, this is always the person you're talking to no matter the direction of the filetransfer. for OUTGOING transfers to conferences, it's one of the recipients (conferences have MULTIPLE rows for the same file, each with a different partner, to differentiate the transfer states for each person; there's also partner_handle for their SkypeID if that is more desirable). for INCOMING transfers from someone in a conference, it's seems to be the name of the sender, but that doesn't matter for us since we already have a reliable sender name via the "from_dispname" row. THE *ONLY* THING WE WILL BE USING PARTNER_DISPNAME FOR IS TO DIFFERENTIATE THE RECEIVING STATE PER-PERSON *OF OUR OUTGOING TRANSFERS TO THE CONFERENCE*.
										
										status (integer): shows the current state of the file, such as transfer completed, or "waiting", etc. known values:
											0 = no action taken
											2 = you are sending a file to someone, they have not accepted yet (this will happen if you send while they are offline or you log out before they accept it, or if you export the database while skype is running and they haven't accepted yet)
											7 = recipient (you/they) cancelled
											8 = recipient (you/they) completed transfer
											9 = ? FIXME: might mean transfer failed due to connection problem. insufficient data to determine what this means.
											10 = skype is currently open. they are trying to send you a file, you have not accepted yet. turns into 0 if you close skype without accepting it.
											12 = you are sending a file to someone, THEY hit cancel. strange, what's different from 7?
											* There are way too many weird codes. I'll simply go 8 = Completed, Everything Else = Not transferred.
										
										note: we COULD also print the "filepath" field, which is a string value showing the source/destination path on OUR disk. this contains stuff like "/Volumes/Secondary OS/path/to/file.jpg"... kinda nice to have but then again the surrounding discussion reveals what the subject was and lets you determine what the files were about, and also by the time you've made the log the files have probably been either moved or deleted in most cases. finally, printing such info would clutter the log files, and storing it as <a href="file://path"> would be nonsensical as A) most paths would be broken and B) the paths could contain sensitive info and leak such info if you share the log with someone.
									*/
									if (!$bIsConference || $thisEventDirection == "I") { // if this is a 1on1 chat, or an incoming event in a conference, simply add the file info as-is, as we know there won't be duplicates
										$iTotalFiles++;
										$iTotalSize += $fileinfo["filesize"]; // add file size to total size
									} else { // it is an outgoing event in a conference, guaranteed by the previous if-statement; we must verify that we don't count the same files twice
										if (!array_key_exists($fileinfo["filepath"], $aConferenceOutgoingFiles)) { // this file has never been encountered before during this transfer chunk
											$iTotalFiles++;
											$iTotalSize += $fileinfo["filesize"]; // add file size to total size
											$aConferenceOutgoingFiles[$fileinfo["filepath"]] = true; // mark this file as seen, to avoid counting it more than once
										}
									}
									
									$aFileLines[] = "<div class=\"t68_f\">".($fileinfo["status"] == "8" ? "<div class=\"icons ft_ok\"><span>OK</span></div>" : "<div class=\"icons ft_failed\"><span>FAILED</span></div>")." ".$fileinfo["filename"]." (".$this->formatBytes($fileinfo["filesize"]).")".($bIsConference && $thisEventDirection == "O" ? " to ".$fileinfo["partner_dispname"] : "")."</div>"; // generate the XHTML-line for the file
								}
								
								// generate summary
								if (!$bIsConference) { // regular chats
									$sMessageBody = ($thisEventDirection == "I" ? $row["from_dispname"]." is sending you ___SUMMARY___:" : "Sending ___SUMMARY___ to ".$_chatParticipants[$_lastDialogPartnerSkypeID].":");
								} else { // conferences
									$sMessageBody = ($thisEventDirection == "I" ? $row["from_dispname"]." is sending ___SUMMARY___ to the conference:" : "Sending ___SUMMARY___ to the conference:");
								}
								$sMessageBody = "<div class=\"t68_h\">".str_replace("___SUMMARY___", ($iTotalFiles == 1 ? "a file" : $iTotalFiles." files")." (".$this->formatBytes($iTotalSize).")", $sMessageBody)."</div>";
								
								// append the fileinfo lines
								$sMessageBody .= join("", $aFileLines);
								break;
							case "30": // call start
							case "39": // call end
								/*
									$_callDetails[$row["convo_id"]] contains the following info for the *latest* call for the convo_id (descriptions replicated from the call-info saver above):
									
									duration (integer): the call duration in seconds if picked up, otherwise null
									
									is_incoming (integer): 1 if true 0 if false
								*/
								$bNoAnswer = ($_callDetails[$row["convo_id"]]["duration"] == NULL ? true : false); // duration is null = no answer (declined/timed out)
								$bIsIncoming = ($_callDetails[$row["convo_id"]]["is_incoming"] == 1 ? true : false);
								
								$sMessageBody = "<div class=\"t".$row["type"]."_c\">"; // t30_c for call start and t39_c for call end
								if ($row["type"] == "30") { // call start
									$sMessageBody .= "<div class=\"callicons call_".($bIsIncoming ? "incoming" : "outgoing")."\"><span></span></div> ";
									if (!$bIsConference) { // regular chats
										$sMessageBody .= ($bIsIncoming ? "Incoming Call from ".$row["from_dispname"] : "Calling ".$_chatParticipants[$_lastDialogPartnerSkypeID])."...";
									} else { // conferences
										$sMessageBody .= ($bIsIncoming ? "Incoming Conference Call from ".$row["from_dispname"] : "Calling Conference")."...";
									}
								} else { // call end
									$sMessageBody .= "<div class=\"callicons call_".($bNoAnswer ? "noanswer" : "ended")."\"><span></span></div> ";
									$sMessageBody .= "Call ended".($bNoAnswer ? ", no answer." : ". Duration: ".$this->callDurationToString($_callDetails[$row["convo_id"]]["duration"])."."); // note: we use the same "call ended" message for both conferences and regular calls. it looks good.
								}
								$sMessageBody .= "</div>";
								break;
							case "10": // person(s) added to conference; can be you or can be others
								if (!$bIsConference) { // regular chats
									// If we see this event and $bIsConference is false then it MEANS *WE* created it, because that is the ONLY time a type10 event occurs with a dialog_partner of the person you're talking to.
									$sMessageBody = $row["from_dispname"]." created a separate conference \"<a href=\"#conf_".$row["convo_id"]."\">".$this->getConferenceName($row["convo_id"])."</a>\".";
								} else { // conferences
									// author/from_dispname = the person that added others. identities = space-separated list of everyone that was added (will only have more than 1 name when YOU created the conference with 2+ people at once (but not when others do that; in that case you get ONE entry in type10 which is your own name, and then a type100 with the other names), OR when someone adds 2+ people in one go while INSIDE the conference).
									
									// Process the identities to give us a nice list to output
									$aIdentities = explode(" ", $row["identities"]); // turn the space-separated skypeID list into an array
									for ($i=0, $len=count($aIdentities); $i<$len; $i++) {
										if (array_key_exists($aIdentities[$i], $_chatParticipants)) // if we've seen this person in the current conference/chat already (unlikely though, as that would require them to leave and then be re-added), then use that cached name
											$aIdentities[$i] = $_chatParticipants[$aIdentities[$i]];
										//FIXME: CONTACT CACHE ONLY EXISTS IN C++ IMPLEMENTATION elseif () // if we have this person in our contact cache (which we should), then use the last name stored in that; alternatively, update the contact cache so it stores the earliest timestamp of each name, then make a function to "getNameAtTimestamp(skypeID, timestamp)" which loops through the names for htat skypeid, backwards, until the earliest-timestamp for it is no longer too-new. if that fails, grab the earliest-known name for that person. store it in the _chatParticipants map.
										// "else": use the skypeID instead; this is what Skype does too when it doesn't know a contact yet.
									}
									$sIdentities = join(", ", $aIdentities);
									
									// Output the merged identities list.
									$sMessageBody = $row["from_dispname"]." added ".$sIdentities." to the conference.";
								}
								break;
							case "13": // person left the conference; can be you or can be others
								// output the name of the person that left the conference; since this event only happens in conferences (not in regular chats), we don't need to check the "isConference" flag, and we can use the message direction to determine whether we left or if it was someone else (we could also check the author field for our skypeID but why bother? event direction tells us what we need to know).
								$sMessageBody = ($thisEventDirection == "I" ? $row["from_dispname"]." has" : "You")." left the conference.";
								
								break;
						}
						
						
						// Output the message
						echo	"				<div class=\"E t".$row["type"]."\">" .
									"<div class=\"M\">".$sMessageBody."</div>" . // the message body
									"<div class=\"T\">".($row["chatmsg_status"] == "1" ? "Pending" : $sDisplayTime)."</div>" . // timestamp including edited/pending state
								"</div>".PHP_EOL;
						break;
					default:
						//echo "Unsupported message type: ".$row["type"];
						//var_dump($row);
						//die(); // FIXME: uncomment if you want to add support for the few missing events (notably 100 for conference participant sync (Skype itself does not render that, though), and 50/51 for friend requests... really, everything of importance is fully supported already)
				}
			}
			if ($_chunkOpen) { echo "			</div>".PHP_EOL."		</div>".PHP_EOL; $_chunkOpen = false; } // close the previous <div class="MC"> message-chunk container if one is open (note: one will always be open at the end, so this is always true)
			echo	"	</div>".PHP_EOL; // closes the final <div class="DayContainer">
			
			
			// End!
			$fEnd = microtime(true);
			$fParseTime = $fEnd - $fStart;
			
			
			// Footer
			echo	"	<div class=\"HistoryFooter\">".PHP_EOL . // starts the history-footer which simply contains the generation time.
					"		<div class=\"LogFooterLine\">Generation took ${fParseTime} seconds.</div>".PHP_EOL . // the log generation runtime
					"	</div>".PHP_EOL . // closes the history-footer
					"</div>".PHP_EOL; // closes the main <div class="ChatHistory"> log-container for this person
			
			
			// If this is a chat with a regular person, we need to locate and output all conferences where they took part.
			// This also includes conferences where they were added but never spoke/caused any events.
			// They're should be sorted by creation date, since we're allowing them to remain sorted by ID, and the ID is an incrementing column describing each conference. It's not impossible that newer IDs can be lower than old ones (since IDs seem quite arbitrary and always make huge leaps in sequence numbers), but that's something we'll notice if that really is the case.
			if (!$bIsConference) { // FIXME: replace this with robust C++ conferences handling instead, where we build a lookup table once, containing all valid conference-IDs and the people in that conference.
				// grab all conversations this SkypeID ever took part in
				$convo_ids_query = $this->db->prepare("SELECT DISTINCT convo_id FROM Messages WHERE (author='".$this->db->escapeString($searchValue)."' OR identities LIKE '%".$this->db->escapeString($searchValue)."%') AND convo_id!=0");
				$convo_ids_result = $convo_ids_query->execute();
				
				// iterate through the conversation IDs and verify that they're really conferences; and if so, output them
				$convo_type_query = $this->db->prepare("SELECT type FROM Conversations WHERE id=?");
				while ($convo_id = $convo_ids_result->fetchArray(SQLITE3_ASSOC)) {
					$convo_type_query->bindParam(1, $convo_id["convo_id"], SQLITE3_INTEGER);
					$convo_type_result = $convo_type_query->execute();
					$convo_type = $convo_type_result->fetchArray(SQLITE3_ASSOC); // we will only ever have one returned row, so grab it directly
					if ($convo_type["type"] == 2) { // this is a conference
						$this->showHistory($convo_id["convo_id"], true); // output the conference.
					}
					$convo_type_query->reset(); // resets statement to its state prior to execution, and keeps all bindings
				}
			}
		}
		
		/*
			Takes a Skype timestamp and returns an array containing the date and the time separately.
		*/
		private function skypeTimeToString(&$timestamp)
		{
			return array(date("F jS, Y", $timestamp), date("g:i:s A", $timestamp)); // [0] = 7/21/2010, [1] = 11:11:45 PM
		}
		
		private function callDurationToString($seconds)
		{
			$hours = floor($seconds / 3600);
			$minutes = floor($seconds % 3600 / 60);
			$seconds = $seconds % 60;
			
			$output = "";
			if ($hours > 0) {
				$output .= "${hours}h ${minutes}m "; // hours AND minutes even if minutes are 0
			}
			elseif ($minutes > 0) $output .= "${minutes}m ";
			$output .= "${seconds}s";
			
			return $output;
		}
		
		/*
			Takes a Skype message XML string and converts it and all entities to XHTML.
		*/
		private function skypeMessageToXHTML(&$msg)
		{
			// parse emoticons and replace them with the appropriate smiley-div
			// before: <ss type="tongueout">:P</ss>
			// after: <div class="ss tongueout"><span>:P</span></div>
			
			// parse flags and replace them with the appropriate flag-div
			// before: <flag country="se">(flag:SE)</flag> <flag country="es">(flag:ES)</flag> (flag:ZZ) (flag:zz) <flag country="es">(flag:es)</flag> <flag country="se">(flag:se)</flag>
			// after: <div class="flag se"><span>(flag:SE)</span></div> and so on...
			
			// then add <br /> in every place that there's a newline
			// lastly, remove all actual newline characters (for output cleanliness), since they've now been turned into <br />
			// warning: do not be tempted to trim() the string to remove start/end whitespace in the message, as messages are legitimately allowed to begin with whitespace indentation.
			// sidenote: Skype stores tabs in the database as-sent/received, but it *displays* them in the chat by converting them to a sequence of 3 spaces, rather than the universal standards of 2/4 spaces (usually 4), which is a bit weird. We *could* behave exactly like skype by converting "\t" to "   " for display purposes, but I actually count this conversion as a fault of Skype and prefer to keep the tabs as-is, to preserve message formatting (particularly when code has been pasted into the chat).
			return	str_replace(array("\r", "\n"), "", nl2br(
						preg_replace( // flag (skype country flags)
							"/<flag country=\"([^\"]+)\">([^<]+)<\/flag>/",
							"<div class=\"flag $1\"><span>$2</span></div>",
							preg_replace( // ss (skype smileys) aka emoticons
								"/<ss type=\"([^\"]+)\">([^<]+)<\/ss>/",
								"<div class=\"ss $1\"><span>$2</span></div>",
								$msg
							)
						)
					));
		}
		
		/*
			Turns a filesize in bytes into a human-readable string. Optimized for speed (no floor()/log()/loop/etc).
		*/
		public function formatBytes($bytes)
		{
			if ($bytes < 1024) return $bytes.' B';
			elseif ($bytes < 1048576) return round($bytes / 1024, 2).' KB';
			elseif ($bytes < 1073741824) return round($bytes / 1048576, 2).' MB';
			elseif ($bytes < 1099511627776) return round($bytes / 1073741824, 2).' GB';
			else return round($bytes / 1099511627776, 2).' TB';
		}
	}

?>
