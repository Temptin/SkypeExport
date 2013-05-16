#include "skypeparser.h"

namespace SkypeParser
{
	#include "../resources/style_compact_data_css.h"

	void CSkypeParser::exportUserHistory( const std::string &skypeID, const std::string &targetFile )
	{
		std::ofstream xhtmlFileWriter( targetFile.c_str(), std::ios::out ); // NOTE: this is NOT a binary output stream, as we want \n to be translated to the appropriate newlines for the platform, in case the user wants to look at the raw log file in an editor that only supports platform newlines.
		if( !xhtmlFileWriter ){ throw std::ios::failure( "error opening html file for writing" ); }
		
		// page header
		xhtmlFileWriter	<< "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
						<< "<html>\n"
						<< "	<head>\n"
						<< "		<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
						<< "		<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\" /> <!-- tells IE to use standards-compliant mode and the newest engine -->\n"
						<< "		<title>Skype History for " << getDisplayNameAtTime( skypeID, -1 ) << "</title>\n" // puts the person's latest displayname in the page title FIXME: verify that Skype has already escaped < > in the database (as &lt; and &gt;), otherwise we must do this replacement manually; this ALSO applies for ALL other places that the displayname is output
						<< "		<style type=\"text/css\">\n"
						<< "			" << style_compact_data_css << "\n"
						<< "		</style>\n"
						<< "	</head>\n"
						<< "	<body>\n";

		// grab the main conversation history for the person
		xhtmlFileWriter	<< getHistoryAsXHTML( (void *)skypeID.c_str(), false ) << "\n";

		// output all conferences the person took part in
		std::vector<int32_t> confs = getConferencesForSkypeID( skypeID );
		for( size_t i=0, len=confs.size(); i < len; ++i ){
			xhtmlFileWriter	<< getHistoryAsXHTML( (void *)&confs[i], true ) << "\n";
		}

		// page footer
		xhtmlFileWriter	<< "	</body>\n"
						<< "</html>";

		// any errors?
		if( !xhtmlFileWriter.good() ){ throw std::ios::failure( "error while writing to html file" ); }

		// done!
		xhtmlFileWriter.close();

		// FIXME: analyze these steps and delete file if already partially created; this requires some care to not delete files that existed before the function was called
	}

	std::string CSkypeParser::formatTime( const struct tm *timestamp_tm, uint8_t format ) // format 0 = "July 8th, 2010"; format 1 = "8:05:07 AM" (no zero-padding of hour); format 2 = "08:05:07" (zero-padding of hour)
	{
		std::stringstream timeOutput( std::stringstream::in | std::stringstream::out ); // holds the date string as it's being constructed
		switch( format )
		{
		case 0: // "July 8th, 2010"
			// Full name of Month
			switch( timestamp_tm->tm_mon )
			{
			case 0: timeOutput << "January"; break;
			case 1: timeOutput << "February"; break;
			case 2: timeOutput << "March"; break;
			case 3: timeOutput << "April"; break;
			case 4: timeOutput << "May"; break;
			case 5: timeOutput << "June"; break;
			case 6: timeOutput << "July"; break;
			case 7: timeOutput << "August"; break;
			case 8: timeOutput << "September"; break;
			case 9: timeOutput << "October"; break;
			case 10: timeOutput << "November"; break;
			case 11: timeOutput << "December"; break;
			}

			// Day Number
			timeOutput << " " << timestamp_tm->tm_mday;
			// Ordinal Day Suffix
			if( timestamp_tm->tm_mday == 1 || timestamp_tm->tm_mday == 21 || timestamp_tm->tm_mday == 31 ){
				timeOutput << "st"; // 1st, 21st, 31st
			}else if( timestamp_tm->tm_mday == 2 || timestamp_tm->tm_mday == 22 ){
				timeOutput << "nd"; // 2nd, 22nd
			}else if( timestamp_tm->tm_mday == 3 || timestamp_tm->tm_mday == 23 ){
				timeOutput << "rd"; // 3rd, 23rd
			}else{
				timeOutput << "th"; // everything else: #th
			}

			// Year
			timeOutput << ", " << ( timestamp_tm->tm_year + 1900 );

			// All done! "July 8th, 2010"
			break;
		case 1: // "8:05:07 AM" (no zero-padding of hour)
		case 2: // "08:05:07" (zero-padding of hour)
			// Hour (is in the range of 0-23, so must be pre-processed differently depending on output format)
			if( format == 1 ){ // 12 hour format; needs adjustment to 12 hour time
				uint32_t hour = timestamp_tm->tm_hour; // make a copy to avoid modifying the original timestamp structure
				if( hour > 12 ){ // if the hour is OVER 12 (meaning 13+), we must subtract 12 hours to convert from 24h to 12h clock
					hour -= 12;
				}else if( hour == 0 ){ // otherwise if it's midnight, set hour to 12 since midnight is 12 AM.
					hour = 12;
				}
				timeOutput << hour << ":";
			}else{ // 24 hour format (format == 2); needs zero-padding for low numbers
				if( timestamp_tm->tm_hour < 10 ){ // hour 0-9 needs zero-padding
					timeOutput << "0";
				}
				timeOutput << timestamp_tm->tm_hour << ":"; // output the hour as-is, since it's in the range of 0-23. our zero-padding above takes care of padding low numbers.
			}

			// Minutes and Seconds (always zero-padded)
			if( timestamp_tm->tm_min < 10 ){ // minute 0-9 needs zero-padding
				timeOutput << "0";
			}
			timeOutput << timestamp_tm->tm_min << ":"; // minutes are in the range of 0-59
			if( timestamp_tm->tm_sec < 10 ){ // second 0-9 needs zero-padding
				timeOutput << "0";
			}
			timeOutput << timestamp_tm->tm_sec; // seconds are in the range of 0-59

			// AM/PM marker if this is 12 hour format
			if( format == 1 ){
				timeOutput << " " << ( timestamp_tm->tm_hour >= 12 ? "PM" : "AM" ); // will be AM from hours 0-11 (midnight to before noon), PM from hours 12-23 (noon or later).
			}

			// All done! "8:05:07 AM" if 12h format or "08:05:07" if 24h format
			break;
		}
		return timeOutput.str();
	}

	/*
		Turns a filesize in bytes into a human-readable string with two decimals of precision. Optimized for speed (no log(), pow(), loops or other crap).
		Works in base 1024 and supports B, KB, MB, GB and TB. More can be added easily but is totally pointless for our needs.
		Returns result as "MB" instead of "MiB" but uses "MiB" base. This is because people expect "MB" with base 1024.
	*/
	std::string CSkypeParser::formatBytes( uint64_t bytes, bool allowZeroTrail )
	{
		static const char *suffix[] = { "B", "KB", "MB", "GB", "TB" };

		int exp;
		long double result; // use a 12 byte float on platforms that care (it's still 8 bytes on some platforms); the largest float we can use to avoid overflow
		if( bytes < 1024 ){ exp = 0; result = (long double)bytes; }
		else if( bytes < 1048576 ){ exp = 1; result = (long double)bytes / 1024; }
		else if( bytes < 1073741824 ){ exp = 2; result = (long double)bytes / 1048576; }
		else if( bytes < 1099511627776ULL ){ exp = 3; result = (long double)bytes / 1073741824ULL; }
		else{ exp = 4; result = (long double)bytes / 1099511627776ULL; }
		
		char tmp[24]; // buffer size rationale: numeric_limits<uint64_t>::digits10 == 19 (19 digits can hold the full 0-9 range), but that's not UINT64_MAX, the first digit can be a 1, so a uint64_t can be up to 20 digits long; then we add 1 for space, 2 for suffix, and 1 for termination.
		sprintf( tmp, "%1.2f", (double)result ); // cast back to double since gcc's sprintf expects that type for "%f"
		char *p = &tmp[strlen( tmp ) - 1]; // address of the last element in the string (not the null byte)
		if( !allowZeroTrail ){ // preserves 123.17 but turns things like 123.10 into 123.1 and 123.00 into 123, for more human-friendly results
			while( (*p) == '0' ){ --p; if( (*p) == '.' ){ --p; break; } } // ends up pointing to the last element to keep (not the null byte) when done
		}
		*(++p) = ' '; // add space separator
		for( const char *psuffix = suffix[exp]; (*(++p) = *(psuffix++)) != '\0'; ); // copy suffix and null terminator from suffix
		
		return std::string( tmp ); // C++ glue... C++ elitists are gonna hate me for this
	}

	/*
		Converts seconds into a string with hours, minutes and seconds (in a compact format which doesn't output hours or minutes unless needed).
		It accepts a 32 bit unsigned integer, which is plenty enough seconds for ALL our needs.
	*/
	std::string CSkypeParser::formatDuration( uint32_t duration_sec )
	{
		uint32_t hours = duration_sec / 3600;
		uint32_t minutes = ( duration_sec % 3600 ) / 60;
		uint32_t seconds = duration_sec % 60;

		std::stringstream durationOutput( std::stringstream::in | std::stringstream::out ); // holds the duration string as it's being constructed
		if( hours > 0 ){ durationOutput << hours << "h " << minutes << "m "; } // hours AND minutes even if minutes are 0
		else if( minutes > 0 ){ durationOutput << minutes << "m "; }
		durationOutput << seconds << "s";

		return durationOutput.str();
	}

	/*
		Takes a Skype message XML string and converts it and all entities to XHTML.
	*/
	std::string CSkypeParser::skypeMessageToXHTML( const char *msgText )
	{
		// initialize the string to be processed from the provided message
		std::string bodyXML = msgText; // we have no choice but to do a copy, as the original is a dynamically allocated char array

		// NOTE: all replacements below are case-sensitive, since we can trust the Skype log to use all-lowercase for these special tags. this saves us some regex processing time since each character only has to be checked once.

		// ss (skype smileys) aka emoticons
		// parses emoticons and replaces them with the appropriate smiley-div
		// before: <ss type="tongueout">:P</ss>
		// after: <div class="ss tongueout"><span>:P</span></div>
		static const boost::regex rgxSmiley( "<ss type=\"([^\"]+)\">([^<]+)</ss>" ); // ss (skype smileys) aka emoticons
		static const std::string subSmiley( "<div class=\"ss $1\"><span>$2</span></div>" );
		bodyXML = boost::regex_replace( bodyXML, rgxSmiley, subSmiley, boost::regex_constants::match_default | boost::regex_constants::format_perl );

		// flag (skype country flags)
		// parses flags and replaces them with the appropriate flag-div
		// before: <flag country="se">(flag:SE)</flag> <flag country="es">(flag:ES)</flag> (flag:ZZ) (flag:zz) <flag country="es">(flag:es)</flag> <flag country="se">(flag:se)</flag>
		// after: <div class="flag se"><span>(flag:SE)</span></div> and so on...
		static const boost::regex rgxFlag( "<flag country=\"([^\"]+)\">([^<]+)</flag>" ); // flag (skype country flags)
		static const std::string subFlag( "<div class=\"flag $1\"><span>$2</span></div>" );
		bodyXML = boost::regex_replace( bodyXML, rgxFlag, subFlag, boost::regex_constants::match_default | boost::regex_constants::format_perl );

		// then replace all newlines (regardless of newline type) with <br /> to get newlines without compromising the cleanliness of the XML output
		// Skype seems to be storing newlines in the same format as the platform you're running while it stores the event; i.e. Mac or Linux would store \n. Windows would store \r\n.
		// this has only been observed with a Mac Skype database, but that database contained \n for just about all events. it did however have SOME \r\n sequences in some of the messages.
		// the messages containing \r\n sequences were mainly <legacyquote> and <quote> blocks WE had received from a Windows user, BUT that Windows user's REGULAR incoming messages did NOT use \r\n, so I am guessing that Skype sends <quote>/<legacyquote> data as-is, but converts incoming newlines to the platform-specific ones for all other messages.
		// however, there were ACTUALLY some OUTGOING messages FROM US that used \r\n despite being on a Mac.
		// I've even seen regular OUTGOING messages containing BOTH \n and \r\n. it's a strange mess, but our regexp takes care of it all with 100% accuracy.
		// they should have just designed it to always insert \n. then again I've seen lots of bad design flaws in the Skype protocol so why am I surprised?
		static const boost::regex rgxNewlines( "(\r\n?|\n\r?)" ); // newlines regardless of type/system, even mixed newlines from different systems in the same string. is greedy, greedy, so it will try \r\n (Win), then \r (<2000 Mac), then \n\r (some weird files can have this), then \n (Mac OS X, UNIX, Linux).
		static const std::string subNewlines( "<br />" ); // in our case, we want <br /> instead of literal newlines since the target format is XHTML
		bodyXML = boost::regex_replace( bodyXML, rgxNewlines, subNewlines, boost::regex_constants::match_default | boost::regex_constants::format_perl );
		
		// NOTE: do NOT be tempted to trim whitespace from the start/end of messages or long stretches of whitespace (although Skype itself neither displays (on screen) nor stores trailing whitespace in the db in the first place, so messages are already filtered nicely in that respect). messages MUST be output as-is to preserve formatting, indentation and things like that.
		// SIDENOTE: Skype stores tabs in the database as-sent/received, but it *displays* them in the chat by converting them to a sequence of 3 spaces, rather than the universal standards of 2/4 spaces (usually 4), which is a bit weird. We *could* behave exactly like skype by converting "\t" to "   " for display purposes, but I actually count this conversion as a fault of Skype and prefer to keep the tabs as-is, to preserve nice looking message formatting (particularly when code has been pasted into the chat and will later be copied from the parsed log).

		// return the processed string
		return bodyXML;
	}

	/*
		Generates the XHTML structure for a single conversation or conference.
		Doesn't include the overall page structure, and is not the function you want if you just want the entire history for a person. Use exportSkypeHistory() for full, formatted output.
		NOTE: searchValue should be const char* for regular chats or int32_t* for conferences and the function interprets them as one or the other depending on the value of isConference.
	*/
	std::string CSkypeParser::getHistoryAsXHTML( void *searchValue, bool isConference )
	{
		int rc; // sqlite return codes
		sqlite3_stmt *pStmt;
		std::stringstream xhtmlOutput( std::stringstream::in | std::stringstream::out ); // holds the xhtml as it's being constructed


		// prepare search variables; conference: partnerid="", convoID=id of conference; otherwise (1on1): partnerid="person we're exporting", convoID=0.
		std::string partnerID = ( isConference ? "" : reinterpret_cast<const char *>( searchValue ) );
		int32_t convoID = ( isConference ? *(reinterpret_cast<int32_t *>( searchValue )) : 0 );


		// name tracker initialization
		// for conferences, this keeps track of the changes in author and displayname over time, to detect changes in who is speaking. in other words, it tells you the details of the latest person to send an incoming event of a supported type.
		// for 1on1 chats, the skypeID will be statically stored and we will only be looking for changes in their displayname.
		// in either case, the myDispName variable always keeps track of changes in *our* displayname.
		// NOTE: the only time we will ever need the partner's displayname is in 1on1 chats when we are calling or sending files to the person, so that's the only value where we care to initialize it properly. the rest of these values are just used to detect author/name changes to know when to output new messagechunks, and are therefore fine to initialize to blank.
		struct {
			std::string skypeID;
			std::string dispName;
		} lastDialogPartner;
		lastDialogPartner.skypeID = ( isConference ? "" : partnerID ); // init partner's skypeid to blank on conference, or to static partnerID on 1on1 chat (no code will modify this value during 1on1 runtime, hence it's static and can be relied on to be the skypeID of your chat partner; alternatively you can use the main "partnerID" variable, which is identical; use partnerID in SQL/header output, and use lastDialogPartner.skypeID in event parsing)
		lastDialogPartner.dispName = ( isConference ? "" : getDisplayNameAtTime( partnerID, 0 ) ); // init partner's dispname to blank on conference, or to their earliest displayname on 1on1 chat // FIXME: if we ever support partial history export (via history ranges), then this will need to be modified a bit to grab their name at that point in time instead
		std::string myDispName = ""; // init our dispname to blank string to detect first "name change" once we see our own name

		/*
			DisplayName Lookup Rules:
				Sender:
					INCOMING: always use $row["from_dispname"] to get the name of the sender.
					OUTGOING: always use "You" or similar "Sending X to" grammar. for cases where your name should be part of the message (such as /me strings), use $row["from_dispname"].
					
				Target:
					WHERE TARGET IS A CONFERENCE: always use "to the conference".
					FOR EVENTS THAT REQUIRE "identities" PARSING: Use getDisplayNameAtTime(skypeID[taken from identities], timestamp of the event), or fall back to the raw skypeID if the displayname query gave no result ("")
					FOR REGULAR CHATS: use lastDialogPartner.dispName to get the most current name of your chat partner.
		*/


		// create pre-compiled statements for file transfer and call information lookups, since a large database can contain thousands of file transfers and will therefore be a significant bottleneck if it has to constantly re-build the statement during the lifetime of the event it's parsing.
		sqlite3_stmt *pTransferInfoStmt, *pCallInfoStmt;
		sqlite3_prepare_v2( mDB, "SELECT filename, filesize, filepath, partner_dispname, status FROM Transfers WHERE chatmsg_guid=? ORDER BY id ASC", -1, &pTransferInfoStmt, NULL ); // NOTE: the parsing of these statements will never fail, since it's just doing parsing here (not actual evaluation), so no need for error checking
		sqlite3_prepare_v2( mDB, "SELECT duration FROM Calls WHERE (conv_dbid=? AND begin_timestamp<=?) ORDER BY begin_timestamp DESC LIMIT 1", -1, &pCallInfoStmt, NULL );


		// Header NOTE: There are conditions in so many places that the repetition cannot be helped, as ternary would be really messy.
		time_t exportTime = time( NULL ); // grabs the current unix timestamp (UTC seconds since January 1st, 1970). we will use this for the header date/time.
		if( !isConference ){ // regular chat
			xhtmlOutput	<< "<div class=\"ChatHistory\">\n" // starts the main log-container for this person
						<< "	<div class=\"HistoryHeader\">\n" // starts the history-header which has some brief info about the generated log
						<< "		<div class=\"SkypeLogo\"></div>\n" // the Skype logo
						<< "		<div class=\"LogHeaderLine1\">Chat History with <a href=\"skype:" << partnerID << "\">" << getDisplayNameAtTime( partnerID, -1 ) << " (" << partnerID << ")</a></div>\n" // clickable name for the person whose history you're exporting (always use the absolute latest displayname for the header)
						<< "		<div class=\"LogHeaderLine2\">Created on " << formatTime( localtime( &exportTime ), 0 ) << " at " << formatTime( localtime( &exportTime ), 1 ) << ".</div>\n" // the log creation date // FIXME: it would be NICE adding a sentence "All times are in UTC+1" (including any active DST at the time of log generation), maybe add that later...
						<< "	</div>\n"; // closes the history-header
		}else{ // conference
			xhtmlOutput	<< "<div id=\"conf_" << convoID << "\" class=\"ChatHistory ConferenceHistory\">\n" // starts the main log-container for this conference (contains anchor-ID)
						<< "	<div class=\"HistoryHeader\">\n" // starts the history-header which has some brief info about the generated log
						<< "		<div class=\"SkypeLogo\"></div>\n" // the Skype Conference logo
						<< "		<div class=\"LogHeaderLine1\">Conference History for \"" << getConferenceTitle( convoID ) << "\"</div>\n" // title of the conference room, grabbed from the database
						<< "		<div class=\"LogHeaderLine2\">Created on " << formatTime( localtime( &exportTime ), 0 ) << " at " << formatTime( localtime( &exportTime ), 1 ) << ".</div>\n" // the log creation date // FIXME: it would be NICE adding a sentence "All times are in UTC+1" (including any active DST at the time of log generation), maybe add that later...
						<< "	</div>\n"; // closes the history-header
		}


		// Start!
		clock_t clock_start, clock_end;
		clock_start = clock();


		// Grab Messages
		// build conference or 1on1 query
		std::string logQuery = "SELECT type, chatmsg_status, author, from_dispname, body_xml, timestamp, edited_timestamp, guid, convo_id, identities FROM Messages WHERE ";
		if( !isConference ){
			logQuery.append("dialog_partner=?"); // grabs all messages and status nodes to/from the person with the given skypeid.
		}else{
			logQuery.append("convo_id=?"); // grabs all messages and status nodes to/from the given conference chat.
		}
		logQuery.append(" ORDER BY timestamp ASC");
		// prepare statement
		rc = sqlite3_prepare_v2( mDB, logQuery.c_str(), -1, &pStmt, NULL );
		if( rc != SQLITE_OK ){
			sqlite3_finalize( pStmt );
			return "";
		}
		// bind the appropriate variable based on search type
		if( !isConference ){
			sqlite3_bind_text( pStmt, 1, partnerID.c_str(), -1, SQLITE_TRANSIENT ); // we use SQLITE_TRANSIENT to tell SQLite to make its own private copy of the string, so that we won't have to worry about the data being freed before SQLite is done with it.
		}else{
			sqlite3_bind_int( pStmt, 1, convoID ); // convoID is a 32 bit integer on all systems, so just bind it as-is using sqlite's 32 bit int bind function.
		}


		// Parse Events
		struct ParserState {
			struct tm activeDay; // holds the active year/month/day combo structure; we'll be comparing the tm_mday, tm_mon and tm_year fields (in that order, as day is most likely to change first) to a temporary parsing of each row's ".row_timestamp", to see if the day has changed; if so, we do a copy of that struct and output a new day-block
			uint8_t chunkState; // keeps track of the currently open chunk's direction; 0 if no chunk is open, 2 for outgoing, 4 for incoming.
			size_t eventCount; // this is how many events (of the supported types) we've parsed for the current conversation
			// initialization
			ParserState() {
				chunkState = 0; // init to 0 ("no chunk open")
				activeDay.tm_mday = 1; // range: 1-31 // init these 3 variables to their lowest values, so that our first log-row comparison will mismatch on purpose and cause a new daycontainer to be output; we'll also use "if( tm_year == 0 )" to be able to determine when we are outputting the first daycontainer
				activeDay.tm_mon = 0; // range: 0-11
				activeDay.tm_year = 0; // years since 1900. we want it to start out at 0.
				eventCount = 0; // init to 0 (no events parsed yet)
			};
		} parserState; // keeps track of internal state variables during the parsing; things that won't be changing every row
		struct SkypeEvent {
			// database columns for current row
			int32_t row_type; // integer
			int32_t row_chatmsg_status; // integer

			const char *row_author; // text
			const char *row_from_dispname; // text
			const char *row_body_xml; // text

			time_t row_timestamp; // integer
			time_t row_edited_timestamp; // integer

			const void *row_guid; // blob
			int32_t row_convo_id; // integer

			const char *row_identities; // text


			// state variables for current row (stuff that will be programmatically calculated/filled in based on row values)
			uint8_t direction; // denotes the event direction of the currently-parsed event; 2 for outgoing, 4 for incoming. does not need to be initialized, as it's updated at the start of each row-loop.
			struct tm timestamp_tm; // time structure that's always filled with the parsed version of the current event's original timestamp (thisEvent.row_timestamp), and is usable every time you need to output or work with the date/time representation of the timestamp of this event.
			std::stringstream asXHTML; // returns the event as XHTML; is emptied at the start of each new event and built during the event parsing step, and finally used as part of the output of that event
			
			// initialization
			SkypeEvent() : asXHTML( std::stringstream::in | std::stringstream::out ) { }; // gives the proper parameters to the stringstream
		} thisEvent; // will hold the data for the current event; basically anything that is updated/calculated for every new row we come across
		while( sqlite3_step( pStmt ) == SQLITE_ROW ){
			// store integers and addresses for the current row (NOTE the null-checking rules for any columns that can be null)
			thisEvent.row_type = sqlite3_column_int( pStmt, 0 );
			thisEvent.row_chatmsg_status = sqlite3_column_int( pStmt, 1 );
			thisEvent.row_author = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 2 ) );
			thisEvent.row_from_dispname = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 3 ) );
			thisEvent.row_body_xml = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 4 ) ); // can be NULL extremely rarely, in which case it will be a NULL pointer; has been observed for types 4, 10, 13, 39, 51, 100 and 110 (WE ONLY USE IT FOR EVENTS 61 AND 60, SO WE CAN SAFELY IGNORE THE RISK OF "NULL")
			thisEvent.row_timestamp = sqlite3_column_int64( pStmt, 5 ); // NOTE: we're grabbing the timestamps as int64's even though they should fit into ints, just be aware of that. some platforms may use an int32 time_t instead of int64, but it will probably convert just fine.
			thisEvent.row_edited_timestamp = sqlite3_column_int64( pStmt, 6 ); // is NULL (int 0 due to _int64() converting NULL columns to 0) for every message/event that has not been edited (that will be the majority), in which case it will be set to 0
			thisEvent.row_guid = sqlite3_column_blob( pStmt, 7 ); // can be NULL extremely rarely (seemingly only for pre-Skype5 events, and only rarely at that), in which case it will be a NULL pointer; has been observed for types 30, 39 and 68 (WE ONLY USE THIS VALUE FOR EVENT 68, SO WE MUST INSERT A NULL CHECK AND AVOID TRYING TO LOOK UP FILE TRANSFER INFO WHERE IT'S NULL; HAS ONLY BEEN OBSERVED FOR A FEW SCATTERED PRE-SKYPE5 EVENTS AND IS SURELY A BUG, AS THOSE TRANSFERS WERE LOGGED PROPERLY AND DID HAVE GUIDS IN THE TRANSFERS TABLE, IT WAS JUST THE MESSAGE GUIDS THAT WERE MISSING)
			thisEvent.row_convo_id = sqlite3_column_int( pStmt, 8 );
			thisEvent.row_identities = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 9 ) ); // is NULL for every message/event that doesn't make use of the identities field (that will be the majority), in which case it will be a NULL pointer (ONLY USED FOR EVENT TYPE 10, IN WHICH CASE IT IS NEVER NULL, SINCE THE IDENTITIES ARE THE VALUE OF THE EVENT)
			// clear the asXHTML stringstream's old contents to give this event/row a fresh start
			thisEvent.asXHTML.str( "" );
			/*
				Column/Event Explanations:

				type (integer): exact type of the message, more reliable than trying to understand chatmsg_type+chatmsg_status pairs, since this identifies events clearly, but use in conjunction with the latter.
				
				(NOT USED) chatmsg_type (integer): the type of event. different from the always-reliable "type" above, in that this seems to be some sort of legacy value. we will never need it, as "type" is the true type of the event, and chatmsg_status is the direction, which is all we need. has also been observed to be NULL extremely rarely (might be due to a bug in Skype, as it should NEVER be NULL). 
				
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
						* can be empty, either via editing into an empty string, or via right click > Remove to delete the message. these actions are equivalent in Skype, and editing a string to "" will in fact act like you've just deleted the message, and block you from editing it again. empty strings are stored as a single null character, NOT as a null pointer.
					type:60=/me emote
						chatmsg_type:7 (always)
							chatmsg_status:2=outgoing (delivered), 4=incoming (delivered)
						* the note for type61 above applies here as well; emotes can be edited and removed, but Skype actually doesn't handle this, as their programmers hadn't thought of the possibility of a user editing an emote, so it will not reflect the edited message until you restart Skype, and removed messages are simply shown as "DisplayName ", meaning an empty string.
						* we'll render empty emotes identically to Skype, in the interest of accuracy, even though Skype's behavior is bugged in this respect.
					type:68=file transfer
						chatmsg_type:7=file transfer, I want to be able to say that this is ALWAYS used but there was ONE exception below.
							chatmsg_status:2=outgoing, 4=incoming
						chatmsg_type:18=? FIXME: only seen this ONCE, in my entire Skype history, and it was indeed a file transfer, of an image, making this "18" value very strange.
							chatmsg_status:2=outgoing, 4=incoming
						* my guess is we should rely on type:68 and just ignore chatmsg_type for file transfers. chatmsg_status still shows the actual direction of the transfer.
						* use guid to look up the transfer details from the Transfers table
						* Transfers table description:
							filename (text): the plain name of the file, such as "the.phantom.menace.1080p.mkv"		
							filesize (text, even though you would expect integer, but I guess they did this to avoid having too-small integers to hold the data in-memory?, luckily we can query it as a 64 bit int to have SQLite cast it for us): the size of the file in bytes, such as "2444978"
							filepath (text): the full path to where we saved the file (if receiving) or where it came from (if sending). this contains stuff like "/Volumes/Secondary OS/path/to/file.jpg". it is NULL if it was an incoming transfer that WE never accepted (either because they cancelled it or because we cancelled it), since Skype has no idea where it WOULD have been saved if we'd have accepted. we'll ONLY use this value during SENDING, and only when that's done TO CONFERENCES. when that's the case, we will use it to avoid counting the same file multiple times for each participant. (we will NEVER use this value for receiving, since receiving files in a conference only inserts ONE entry per file in the Transfers table, we don't know the state of the others in the conference and don't need to bother with that; as for outgoing transfers to 1on1 chats, we likewise only have 1 entry per file since there's only 1 recipient; we ONLY need it for OUTGOING TO CONFERENCE). NOTE: I wish we could have just used the participant_count column in the Messages table to divide to remove duplicates, as that would have been such a clean solution, but sadly that value is NULL for pre-Skype5 events...
							partner_dispname (text): the displayname of your chat partner. for 1on1 chats, this is always the person you're talking to no matter the direction of the filetransfer. for OUTGOING transfers to conferences, it's one of the recipients (conferences have MULTIPLE rows for the same file, each with a different partner, to differentiate the transfer states for each person; there's also partner_handle for their SkypeID if that is more desirable). for INCOMING transfers from someone in a conference, it seems to be the name of the sender, but that doesn't matter for us since we already have a reliable sender name via thisEvent.row_from_dispname. THE *ONLY* THING WE WILL BE USING PARTNER_DISPNAME FOR IS TO OUTPUT THE COMPLETION STATE PER-PERSON *OF OUR OUTGOING TRANSFERS TO THE CONFERENCE*.
							status (integer): shows the current state of the file, such as transfer completed, or "waiting", etc. known values:
								0 = no action taken
								2 = you are sending a file to someone, they have not accepted yet (this will happen if you send while they are offline or you log out before they accept it, or if you export the database while skype is running and they haven't accepted yet)
								7 = recipient (you/they) cancelled
								8 = recipient (you/they) completed transfer
								9 = ? FIXME: might mean transfer failed due to connection problem. insufficient data to determine what this means.
								10 = skype is currently open. they are trying to send you a file, you have not accepted yet. turns into 0 if you close skype without accepting it.
								12 = you are sending a file to someone, THEY hit cancel. strange, what's different from 7?
								* There are way too many weird codes. I'll simply go 8 = Completed, Everything Else = Not transferred.
							NOTE: we COULD print the "filepath" field as part of the log output, as it's a string value showing the source/destination path on OUR disk. this contains stuff like "/Volumes/Secondary OS/path/to/file.jpg"... kinda nice to have but then again the surrounding discussion reveals what the subject was and lets you determine what the files were about, and also by the time you've made the log the files have probably been either moved or deleted in most cases. finally, printing such info would clutter the log files, and storing it as <a href="file://path"> would be nonsensical as A) most paths would be broken and B) the paths could contain sensitive info and leak such info if the user shares the log with someone. logs shouldn't contain "hidden" info like that.
					type:30=call start (same _type and _status as 39)
					type:39=call end
						chatmsg_type:18 (always)
							chatmsg_status:2=outgoing, 4=incoming (NOTE: ONLY USABLE FOR CALL START; call_end events haven't got the proper direction status to show who really ended the call and should NOT be used for that)
						* the call_start event happens either A) when they pick up, or B) when the program gives up trying to call them (assumes they're not going to answer, which takes 2 minutes of trying (in Skype5 as of now, may change later))
						* the call_end happens at exactly exactly the same timestamp (OR up to 1 second later) as call_start when they declined/didn't pick up. this means that the timestamp-difference between the end and start of a call is NOT usable to determine if it was picked up or the length of the call. there's up to 1 second of sway, which will cause false "call picked up" positives, and also misreport the length of a call by up to 1 second. the only workarounds are to either A) ignore timestamp differences if they are just 1 second (interpret diff < 2 as "not picked up") which sacrifices duration accuracy and pick-up accuracy (what if the call WAS 1 second in duration?), or B) look up the Call in the calls table. I'm opting for the latter.
						* there is a column, call_guid, which identifies each call for Skype5 but it's NULL for VERY OLD calls performed in Skype versions older than 5, so we cannot rely on it under any circumstance!
						* more data can be found in the Calls table, which has stuff like whether WE MISSED the call by not responding for 2 minutes and they didn't abort it either (that's the is_unseen_missed field; however, as mentioned, it ONLY RELATES TO INCOMING CALLS so if YOU call someone and they don't pick up for 2 minutes, the is_unseen_missed will still be 0. it's only 1 for missed INCOMING calls), and any microphone problems etc. useless info for the most part, but we need to access the data in that table for accurate call picked up/duration information.
						* since the type30 event is logged at the moment the call either fails or is connected, rather than when the call attempt began, it means that Skype's OWN history viewer does the same thing; it shows the start and end events at the exact same point in time in case it was declined/no pickup. we can even improve that, by just saying "call declined". sadly we have no way to look up the time that the call attempt was REALLY STARTED. that data DOES seem to be available in the Calls table, which contains a begin_timestamp that seems to show the time you BEGAN the call attempt. however, it's useless to us since we can't "go back in time" to print the "call attempt" line when we've already gone way past that row in our output construction. that's most likely why Skype's designers also ignore the true call start time and just show the call start/call end next to each other for failed calls.
						* the time it takes for a call to auto-abort is 2 minutes if nobody picks up and nobody clicks anything to abort it ahead of time. this is useless info for us and Skype may change that interval some day, but I'm mentioning it here anyway.
						* strategy: on call start (type 39), look up the call in the Calls table by using convo_id + "call start <= event39 timestamp", sorted by timestamp descending, limit 1. this will pick up the data-line describing that call, since we cannot rely on the call_guid (only exists for Skype5+ events). then, grab the duration from there and use a NULL duration to say "call aborted/missed" or a duration of 1+ to show that the call connected as well as its duration. this works since there can only be one active call at a time per convoID in Skype (if you call someone or they call you, the recipient's call button immediately becomes a "hang up" button), and we're only parsing one convoID at a time.
						* Calls table description:
							(NOT USED) begin_timestamp (integer): the true time that the call event was received; this is the actual first "ring" event. in 2 minutes of ringing with no pickup (in Skype5 as of now, may change later), the regular Messages log will be generating a 30 and 39 event with identical timestamps to say "call of duration 0", which is a very stupid way of logging things. they should have stored the call start at the moment it really happened, and also had call_guid correlate to the Calls table for fuck sake... well, anyway...
							(NOT USED) is_unseen_missed (integer): THIS ONLY RELATES TO *INCOMING* CALLS *TO YOU*. this is 1 if you don't pick up before the call attempt TIMES OUT (2 minutes or so is the timeout), otherwise 0. note that this value is ONLY for actual TIMEOUTS (2 minutes of them trying to call you and you not picking up at all), NOT when the caller (or the callee) aborts the attempt manually!
							duration (integer): the call duration in seconds if picked up, otherwise null
							(NOT USED) is_incoming (integer): 1 if true 0 if false. useless since we only need it for the call start event and for that we can just look at the event direction to know whether it's incoming or outgoing.
							(NOT USED) is_conference (integer): 1 if true, null otherwise
					type:10=person added to group conference; if you were the one that created the conference then the dialog_partner will be the person you were chatting with when you pressed the "add to conference" button, and the convo_id will be the conference ID. if someone ELSE adds you, the convo_id is 0 (except possibly re-used IDs on re-used conferences) and dialog_partner is null. "participant_count" is the number of people in the conference (this number will keep rising as more people are added, and lowering as people leave). when you first join a conference CREATED BY SOMEONE ELSE, an event type "100" is also fired, with an identities field like "coolguy123 otherguy123 grince.farbgold", in other words a space-separated list of all current participants (any people that you don't have on your contact list is added to your Contacts table, with is_permanent=0 to signal that you have not added them to your permanent contacts, but that their data is available for use i.e. getting their displaynames). after that, it's up to you to keep track of joins/leaves/participant counts via events types 13 and 10.
						chatmsg_type:1 (always, apparently)
							chatmsg_status:as usual this means the direction (incoming/outgoing) of the event
								* parsing strategy: parse history for dialog_partner, and at the first sign of type:10 you grab the new convo_id and store it in an array as "conferences started by us", then look up the associated displayname (in the Conversations table) for that conference, and output a chat link saying "from_dispname (our name) started a conference: name of conference". later, after you have parsed the log, you grab any missing conference ids and then finally loop through your "convo_id" (conference) arrays and output those too at the end of the document, with appropriate "<div id=conf_74327>" tags to allow the above conference links to work.
								* also NOTE: the initial event that started the conference is ONLY logged for the person you were chatting with at the time and ONLY IF *YOU* CREATED THE CONFERENCE, NOT for any of the people that were added to the conference, and NOT if THEY invited YOU, so you will not see conference links for those people when viewing their logs; you only ever see links when YOU created the conference, and ONLY for the one person you were chatting with at the time you pressed the "+add people" button.
								* we will not be parsing event 100 (the "people in the conference" sync event), because we don't care who is already in the conference.
								* the "identities" field for event type 10 is USUALLY *one* name because of how people use it (when YOU *and other people* become invited to a conference, meaning when some third party selects you and a bunch of others ot make a conference, then you will get a type 10 with JUST your name in it, and a type 100 with the other names. also, when someone adds JUST ONE person to the conference, you get 1 identity. the trouble happens when someone adds 2+ people after the conference started, OR when *YOU* start the conference with multiple people selected. in those cases, the identities field will have as many space-separated SkypeIDs as needed.
					type:13=person left the conference.
						chatmsg_type:4 (always, apparently)
							chatmsg_status:as usual this means the direction (incoming/outgoing) of the event; if it is incoming then someone else left, and if it is outgoing then you left.
								* this one is really easy to handle, just use the author/from_dispname to show the person that left for incoming, or write "you left" for outgoing. unlike type 10, "identities" is not used at all here, thankfully.
								* this event only occurs inside conference convo_id streams, so no need to check the isConference flag.
				* NOTE: outgoing means from YOU to the partner --->, and incoming means from the PARTNER to you <---
				
				author (text): the SkypeID of the person that wrote the message (such as andy.norin). will NEVER be null.
				
				from_dispname (text): the active displayname of the person that wrote the message (such as Anders Norin). will NEVER be null. FIXME: are < and > converted to html entities already?
				
				body_xml (text): the text that was written. it's fully xml-compliant, so stuff like apostrophes are &apos; (well, there IS a flag called body_is_rawxml which is either 1 or null, but I had a look and the only time it has been null is the times when body_xml has been either empty or null itself, so we can safely assume that body_xml is ALWAYS encoded as safe xml). NOTE: Skype leaves ALL whitespace intact EXCEPT trailing whitespace; so any trailing spaces, tabs or newlines and other types of whitespace are ALL stripped when the message is sent/stored in the DB. this does not apply to prepended whitespace, which is kept as-is!
				
				timestamp (integer): the unix timestamp of when the message was first sent (not when it was delivered, which may come later)
				
				edited_timestamp (integer): null most of the time, but when it contains a value it gives the time of the last edit to the message
				
				(NOT USED) dialog_partner (text): the name of the person you are chatting with in single-person chats. always has the same value even when THEY send YOU a message. example: "andy.norin". this value is null in multiperson chats (conferences), apart from the very first message in a conference where it seems to denote the person you were chatting with when the conference was created.
				
				guid (blob): this is a globally unique identifier for the message. we only need it when parsing filetransfer data, as the filetransfer information is stored under this guid.
				
				(NOT USED) call_guid (text): this is the call globally unique identifier, used for identifying individual calls' start and end events. only contains any data for event types 30+39. WARNING: call_guid is null for VERY OLD calls performed in Skype versions older than 5, so we cannot use it!
				
				convo_id (integer): this is the unique identifier for your dialog partner combination; it is permanent (the same whenever you talk to the same partner). also, whenever you create a conference chat (more than 2 people), it generates a new convo_id. The "Conversations" table contains mappings for id -> partner's "identity" (skypeID) and other info. we'll mainly be using convo_id to correlate calls in the Calls table. LONG NOTE: However, you should NOT use convo_id to "locate all events between you and that person"; use dialog_partner for that, because otherwise you will be lacking some events, primarily (or only? it is the only missing one I have observed) event type 10 aka the creation of a new group conference chat (by inviting more chat partners), as that event will still have a dialog_partner that's the same as the person you were chatting with, but the convo_id will be new (the new convo id for that new group conference, to be precise) - note that this is useful info if you want to parse group convos too, simply store all eventtype 10 convo ids and then parse those too; the first event of a group conference has the dialog_partner you were with when the group conference was started, and the remaining ones have a null value for dialog_partner.
				
				identities (text): this is NULL for EVERY event type EXCEPT a handful. when it's NOT null, it is a space-separated list of one or more skypeIDs of the people that are taking part of that event. here are the eventtypes where it is used: 110 (unknown event). 100: conference sync event (contains list of existing conference participants, seems to only be sent out when you are added to someone else's conference). 51 (unknown event, almost 100% sure it is for accepted friend request). 50 (friend request; status 2/4 indicates direction). 39 (call start, contains list of people called; one in 1on1, one or more in conference). 30 (call end; value similar to call start, except it may possibly contain early dropouts as single identities entries, until finally everyone has dropped out; OR perhaps it just indicates when YOU hung up and the people that were also in that call, even people that dropped out earlier? FIXME: test that? or not give a shit? I prefer NOT GIVE A SHIT, but someone else is free to investigate!). 10 (conference join event; identities contains the list of people that were added, usually just one but will be several if more than one person is added at a time). oddly enough event 13 (conference leave event) does NOT use identities, instead the "author"/"from_dispname" is the person that left (which can be yourself, just use chatmsg_status to judge direction to see if it was you that left)
			*/
			switch( thisEvent.row_type ) // filters out just the events we care about
			{
			case 61: // regular text message
			case 60: // /me emotes
			case 68: // file transfer
			case 30: // call start
			case 39: // call end
			case 10: // person(s) added to conference
			case 13: // person left the conference
				{
				/************************* EVENT PARSING START *************************/


				// If the day/month/year combo of this event differs from our current section, create a new daycontainer. This will also create our initial daycontainer.
				thisEvent.timestamp_tm = *localtime( &thisEvent.row_timestamp ); // parse the current row's timestamp into the temporary, global "tm" struct and then do a member-to-member copy into our row structure (this avoids the risk of other threads or things calling the global time functions and thereby modifying the data)
				if( thisEvent.timestamp_tm.tm_mday != parserState.activeDay.tm_mday ||
					thisEvent.timestamp_tm.tm_mon  != parserState.activeDay.tm_mon  ||
					thisEvent.timestamp_tm.tm_year != parserState.activeDay.tm_year ){
						if( parserState.chunkState != 0 ){ xhtmlOutput << "			</div>\n		</div>\n"; parserState.chunkState = 0; } // close the previous <div class="MC"> message-chunk container if one is open; this will also notify the chunk-output code below that it needs to create a new message chunk due to end of day
						if( parserState.activeDay.tm_year != 0 ) xhtmlOutput << "	</div>\n"; // close the previous <div class="DayContainer">, unless activeDay.tm_year is 0 (meaning this day is the first one we're outputting for the current log, in which case there is nothing to close yet)
						parserState.activeDay = thisEvent.timestamp_tm; // store the active day to prepare for detecting the next day, by doing a member-to-member copy of the current row's tm structure into our activeDay structure; we only care about updating the tm_mday, tm_mon and tm_year fields, but it's neater to just copy the whole thing.
						xhtmlOutput	<< "	<div class=\"DayHeader\">" << formatTime(&parserState.activeDay, 0) << "</div>\n" // output day-marker; date is formatted as "July 8th, 2010"
									<< "	<div class=\"DayContainer\">\n"; // begin a message container for this day
				}
				

				// Grab the chunk direction (1: pending, outgoing. 2: delivered, outgoing. 4: delivered, incoming) and store it as either 2 or 4.
				thisEvent.direction = ( thisEvent.row_chatmsg_status == 1 || thisEvent.row_chatmsg_status == 2 ? 2 : 4 ); // 2 = outgoing, 4 = incoming


				// Determine whether we should make a new chat message container. Reasons for doing so: Day changed (day changes close the chunk and set parserState.chunkState to 0, which is how it knows to make a new container for the new day). Person talking changed (during conference). Message direction changed (incoming/outgoing). Someone (even yourself) changed their displayname (either in 1 on 1 or conference).
				// NOTE: Since this whole code block only triggers for events we actually care about outputting (thanks to the switch above), we are SURE that the event that we're working on will actually be output by us graphically, so it's safe to assume that if we decide to output a new messagecontainer there WILL be AT LEAST 1 event to put in it.
				bool makeNewContainer = false;
				if( thisEvent.direction == 4 ){ // INCOMING EVENTS
					if( isConference && thisEvent.row_author != lastDialogPartner.skypeID ){ // INCOMING EVENTS IN CONFERENCES ONLY: if the INCOMING event author (author=SkypeID) changed (even though the direction MAY not have switched over from incoming to outgoing yet), then we'll need a new messagecontainer; this catches multiple incoming participants in a row in a conference chat. we never perform this lookup during regular 1on1 chats, as we've already initialized it to a static skypeid value if it's a 1on1 chat.
						makeNewContainer = true;
						lastDialogPartner.skypeID = thisEvent.row_author; // 1on1: already contains the static skypeid for the person you are talking to. conference: contains the last INCOMING skypeid, to allow us to notice multiple people.
					}
					if( thisEvent.row_from_dispname != lastDialogPartner.dispName ){ // INCOMING EVENTS ONLY: if the incoming person's displayname has changed, output a new block; this catches name changes of existing people, even mid-stream while they're talking, both in conferences and 1on1.
						makeNewContainer = true;
						lastDialogPartner.dispName = thisEvent.row_from_dispname; // this works because author and from_dispname are never null and always correlate.
					}
				}
				if( thisEvent.direction == 2 && thisEvent.row_from_dispname != myDispName ){ // OUTGOING EVENTS ONLY: if our outgoing displayname has changed, we need to output a new block; this catches changes in our displayname, even mid-stream, both in conferences and 1on1.
					makeNewContainer = true;
					myDispName = thisEvent.row_from_dispname; // stores our latest displayname; will also catch our initial displayname, since we've initialized it to "" which will mismatch the if-statement
				}
				if( thisEvent.direction != parserState.chunkState ){ // if we don't have a chunk at the moment (as will be the case if this is the first event of the whole log), or if the direction has transitioned between incoming/outgoing events, we also need to output a new chunk; this also catches cases in 1on1 chats where our PARTNER is the FIRST person to speak, as their skypeID and earliest displayName are already pre-initialized to the correct values and would not be caught by the above checks, but that's caught here since there will be no open block at this point in time.
					makeNewContainer = true;
					// NOTE: we'll take care of actually updating parserState.chunkState to match during the chunk output below 
				}
				

				// Check whether we should create a new message-chunk container to indicate whoever is speaking/sending the event
				if( makeNewContainer ){ // change of message direction, or a new day has occured and reset the chunkState variable to 0, or the dialog partner or their name has changed while we're still receiving incoming messages (happens in 1on1 if the partner changes name without you sending them any messages inbetween; and in conference chats when different incoming people write to you all in one "incoming" stream without you saying anything), or we've changed our own displayname while still sending outgoing messages...
					if( parserState.chunkState != 0 ){ xhtmlOutput << "			</div>\n		</div>\n"; parserState.chunkState = 0; } // close the previous <div class="MC"> message-chunk container if one is open
					
					parserState.chunkState = thisEvent.direction; // store the active chunk direction
					xhtmlOutput	<< "		<div class=\"MC S" << ( parserState.chunkState == 2 ? "O" : "I" ) << "\">\n" // begin the new message chunk, with the current direction (status) as an appended class
								<< "			<div class=\"A\">" << thisEvent.row_from_dispname << "</div>\n" // author of the message-chunk
								<< "			<div class=\"C\">\n"; // container for holding all the messages in this message-chunk
				}


				// Parse and format the event body as XHTML for display
				switch( thisEvent.row_type )
				{
				case 61: // regular text message
					if( thisEvent.row_body_xml[0] == '\0' ){ // removed messages are stored in the database as an empty string rather than as NULL, so we simply check if the first character is a null character to see if the string is indeed empty.
						thisEvent.asXHTML << "<div class=\"t61_r\">This message has been removed.</div>"; // an empty message means that the sender has hit "remove"
					}else{
						thisEvent.asXHTML << skypeMessageToXHTML( thisEvent.row_body_xml ); // NOTE: this can be an empty string (just like regular text messages above) in case the user has edited it into an empty string, or simply hit Remove. however; Skype itself DOES NOT HANDLE THIS CASE and in fact doesn't even update to reflect the removal until Skype is restarted, upon which it just shows "DisplayName " with no content, so in the interest of accuracy we won't invent our own error string for empty emotes.
					}
					break;
				case 60: // /me emotes
					thisEvent.asXHTML << thisEvent.row_from_dispname << " " << skypeMessageToXHTML( thisEvent.row_body_xml ); // FIXME: read the docs for type60 above to see that Skype is currently not handling empty (removed/edited to "") emotes right now, but if Skype ever starts handling them, this will need to be updated to reflect that.
					break;
				case 68: // file transfer
					{
					// a GUID in the Skype database is a sequence of 32 unsigned bytes. we will be copying the value pointed to by the GUID pointer and storing the result here to use as our GUID later, but first we must verify that our GUID pointer is not null...
					uint8_t transferguid[SKYPEPARSER_GUID_SIZE];
					bool guidFound = false; // will be set to true when we know that we have a valid GUID. this will NEVER remain false, as that would require a bugged file transfer without GUID *AND* the alternate lookup method to fail, which it never will. however, it's better to be safe than sorry.

					// this first check tries to fix an EXTREMELY RARE pre-Skype5 bug where a file transfer event may be logged with a NULL guid. if that's the case, we'll try getting the GUID for this event from the file transfer table instead, as a last resort.
					if( thisEvent.row_guid != NULL ){
						// a valid GUID existed, so just copy it as-is. NOTE: thisEvent.row_guid is a pointer to the 32 bytes of real data!
						std::memcpy( &transferguid, thisEvent.row_guid, SKYPEPARSER_GUID_SIZE * sizeof( uint8_t ) );
						guidFound = true;
					}else{
						// this is such a rare occurence that we can afford to create and delete the extra lookup statement within the lifetime of this event
						sqlite3_stmt *pAlternateGuidStmt;

						// prepare statement
						// NOTE: we cannot rely on the convo_id field of the Transfers table to narrow down the search to prevent matching transfers from separate conversations with exactly the same/extremely similar timestamps, because the convo_id field in the Transfers table is blank almost 50% of the time. therefore, adding a convo_id check adds a hefty risk of not finding any chatmsg_guid at all, so we cannot afford to do it. also, the "chatmsg_guid NOT NULL" is just there for safety; after ~2000 transfers in the Transfers table I have still never seen one with a NULL chatmsg_guid.
						// NOTE: events lacking guid (and requiring this lookup) are rare enough (for my database, it was 3 file transfers out of 1753 over a one year period, all 3 of which were done in pre-Skype5 and looks like a rare bug as they happened with the same person at the same time period), and actually getting transfers from multiple conversations at *exactly the same/very similar* timestamp is even rarer, so we can afford that small risk.
						// NOTE: SKYPE ITSELF *DOESN'T EVEN TRY* *THIS* guid recovery attempt and just refuses to show info for such bugged transfers. most people will never even have *a single* bugged transfer in their database. this workaround is guaranteed to find the message's GUID in almost 100% of cases, with a ridiculously low risk of false matches, so it's good-enough for a very uncommon situation, and I cannot see any better solutions due to the correlation limitations of the available data.
						rc = sqlite3_prepare_v2( mDB, "SELECT chatmsg_guid FROM Transfers WHERE (starttime<=? AND chatmsg_guid NOT NULL) ORDER BY starttime DESC LIMIT 1", -1, &pAlternateGuidStmt, NULL );
						if( rc == SQLITE_OK ){
							// bind parameters
							sqlite3_bind_int64( pAlternateGuidStmt, 1, thisEvent.row_timestamp ); // always treat timestamp as a 64 bit value
							// grab the chatmsg_guid field IF it exists
							if( sqlite3_step( pAlternateGuidStmt ) == SQLITE_ROW ){
								// copy the newfound GUID directly into the destination "transferguid" storage, since the data pointed to by this statement will be freed as soon as sqlite3_finalize() is called below, hence updating the thisEvent.row_guid pointer would be pointless.
								// NOTE: the retrieved GUID is close to 100% guaranteed to be correct. simultaneous transfers in different conversations/conferences at exactly the same/very similar time could throw it off, but that's insanely rare as it would have to happen at the same time.
								std::memcpy( &transferguid, sqlite3_column_blob( pAlternateGuidStmt, 0 ), SKYPEPARSER_GUID_SIZE * sizeof( uint8_t ) );
								guidFound = true;
							}
						}

						// free up statement
						sqlite3_finalize( pAlternateGuidStmt );
					}

					// transferguid should now contain the 32 byte GUID; if not, we'll output something to let the user know gracefully
					if( !guidFound ){
						thisEvent.asXHTML << "<div class=\"t68_h\">File transfer information lost due to corrupt database.</div>"; // this should NEVER happen, thanks to our GUID recovery code above
					}else{
						// alright, we have the GUID and can now look up the transfer details

						// reset the statement so that it begins the search anew (otherwise it would carry on with its state since last time we used the statement)
						sqlite3_reset( pTransferInfoStmt ); // NOTE: this will not clear any existing bindings. to unset bindings you use sqlite3_clear_bindings(). we won't bother with that and will instead simply re-bind the parameter below.

						// bind parameters
						// pTransferInfoStmt = "SELECT filename, filesize, filepath, partner_dispname, status FROM Transfers WHERE chatmsg_guid=? ORDER BY id ASC"
						sqlite3_bind_blob( pTransferInfoStmt, 1, &transferguid, SKYPEPARSER_GUID_SIZE * sizeof( uint8_t ), SQLITE_STATIC ); // SQLITE_STATIC means that SQLite will NOT copy the contents of transferguid when it binds the variable, so we must be VERY careful not to free or modify that memory before we're done using this statement!

						// look up all file entries involved in the transfer via the Transfers table, going by the transfer's GUID.
						std::stringstream fileInfoXHTML( std::stringstream::in | std::stringstream::out ); // this is used for building the XHTML string containing information about all files for this transfer
						int64_t totalsize = 0; // will hold the combined size of all files (64 bit int is absolutely needed here, as the transfer of multiple files could easily go past 2 gigabytes of total size if someone is transferring several video files or disc images)
						uint32_t filecount = 0; // the number of unique files being transferred
						std::list<std::string> confUniqueFiles; // ONLY used during OUTGOING transfers to CONFERENCES, and is used to determine the true count of files being sent and to only count files once
						std::list<std::string>::iterator confUniqueFiles_it;
						struct {
							const char *filename; // text
							int64_t filesize; // text, but we will have sqlite cast it to integer, can contain values > 32 bit integer
							const char *filepath; // text
							const char *partner_dispname; // text
							int32_t status; // integer
						} fileInfo;
						while( sqlite3_step( pTransferInfoStmt ) == SQLITE_ROW ){
							// store integers and addresses for the current file entry (NOTE the null-checking rules for any columns that can be null)
							fileInfo.filename = reinterpret_cast<const char *>( sqlite3_column_text( pTransferInfoStmt, 0 ) );
							fileInfo.filesize = sqlite3_column_int64( pTransferInfoStmt, 1 ); // we must get the size as a 64 bit signed integer, as that is what SQLite's C API uses internally (it does not have unsigned integers), otherwise we will not support files larger than 2 GiB (as rare as such transfers may be, they are legal)
							fileInfo.filepath = reinterpret_cast<const char *>( sqlite3_column_text( pTransferInfoStmt, 2 ) ); // is NULL if you are the RECIPIENT and did not accept the transfer. for our purposes, this does not matter, as we only use this value while SENDING during CONFERENCES.
							fileInfo.partner_dispname = reinterpret_cast<const char *>( sqlite3_column_text( pTransferInfoStmt, 3 ) );
							fileInfo.status = sqlite3_column_int( pTransferInfoStmt, 4 );

							// generate all of the individual file lines, and keep a tally of their combined size and file count
							if( !isConference || thisEvent.direction == 4 ){ // if this is a 1on1 chat, or an incoming event in a conference, simply add the file info as-is, as we know there won't be duplicates
								++filecount;
								totalsize += fileInfo.filesize;
							}else{ // it is an outgoing event in a conference, guaranteed by the previous if-statement; we must verify that we don't count the same files twice
								// count this file if it's the first time we encounter it for this transfer
								confUniqueFiles_it = std::find( confUniqueFiles.begin(), confUniqueFiles.end(), fileInfo.filepath ); // as explained above, this is NEVER NULL for OUTGOING transfers, so this statement is safe
								if( confUniqueFiles_it == confUniqueFiles.end() ){ // this file has never been encountered before during this transfer chunk
									++filecount;
									totalsize += fileInfo.filesize;
									confUniqueFiles.push_back( fileInfo.filepath ); // mark this file as seen, to avoid counting it more than once
								}
							}

							// generate the XHTML-line for the file and append the file's information to the XHTML line storage
							fileInfoXHTML	<< "<div class=\"t68_f\">" << ( fileInfo.status == 8 ? "<div class=\"icons ft_ok\"><span>OK</span></div>" : "<div class=\"icons ft_failed\"><span>FAILED</span></div>" ) << " " << fileInfo.filename << " (" << formatBytes( fileInfo.filesize, false ) << ")";
							if( isConference && thisEvent.direction == 2 ){ // outgoing transfers in conferences can have more than one recipient, so we output the recipient name after the file name in that case
							fileInfoXHTML	<< " to " << fileInfo.partner_dispname;
							}
							fileInfoXHTML	<< "</div>";
						}

						// generate the count/size summary ("X files (18.34 MB)")
						std::stringstream sizeSummaryXHTML( std::stringstream::in | std::stringstream::out );
						if( filecount == 1 ){ sizeSummaryXHTML << "a file"; }
						else{ sizeSummaryXHTML << filecount << " files"; }
						sizeSummaryXHTML << " (" << formatBytes( totalsize, false ) << ")";

						// save the header line
						thisEvent.asXHTML << "<div class=\"t68_h\">";
						if( !isConference ){ // regular chats
							thisEvent.asXHTML << ( thisEvent.direction == 4 ? std::string( thisEvent.row_from_dispname ) + " is sending you " + sizeSummaryXHTML.str() + ":" : std::string( "Sending " ) + sizeSummaryXHTML.str() + " to " + lastDialogPartner.dispName + ":" );
						}else{ // conferences
							thisEvent.asXHTML << ( thisEvent.direction == 4 ? std::string( thisEvent.row_from_dispname ) + " is sending " + sizeSummaryXHTML.str() + " to the conference:" : std::string( "Sending " ) + sizeSummaryXHTML.str() + " to the conference:" );
						}
						thisEvent.asXHTML << "</div>";

						// save the fileinfo lines
						thisEvent.asXHTML << fileInfoXHTML.str();
					}
					break;
					} // end of type68 (file transfer) parsing
				case 30: // call start
				case 39: // call end
					{
					thisEvent.asXHTML << "<div class=\"t" << thisEvent.row_type << "_c\">"; // t30_c for call start and t39_c for call end
					if( thisEvent.row_type == 30 ){ // call start
						thisEvent.asXHTML << "<div class=\"callicons call_" << ( thisEvent.direction == 4 ? "incoming" : "outgoing" ) << "\"><span></span></div> ";
						if( !isConference ){ // regular chats
							thisEvent.asXHTML << ( thisEvent.direction == 4 ? std::string( "Incoming Call from " ) + thisEvent.row_from_dispname : std::string( "Calling " ) + lastDialogPartner.dispName ) << "...";
						}else{ // conferences
							thisEvent.asXHTML << ( thisEvent.direction == 4 ? std::string( "Incoming Conference Call from " ) + thisEvent.row_from_dispname : "Calling Conference" ) << "...";
						}
					}else{ // call end
						// we need to look up the Call information by looking up a matching entry in the Calls table. information exists there even for the oldest, pre-Skype5 calls. this will tell us the duration of the call and whether or not the call was aborted/missed.
						// however, no direct mapping exists, so we'll have to sort calls in reverse by begin_timestamp, where the convo_id is the same, and the begin_timestamp is <= the timestamp of the "call end" event.
						// only 1 call is legal at a time for any particular convo-id; if you call someone, both of your "call" buttons become "hang up" instead, even while the call is pending, and even if you both try calling each other at exactly the same time, so there is no way to have multiple calls at once with the same person (FIXME: not tested with conference calls, where some people may be able to drop out and call again later to re-join?)
						// FIXME: this hinges on there being NO "call start" events while a call is going on, but I must test a conference: call everyone, someone hangs up, that person re-dials while the others are in chat. and then try the same thing except WE hang up and re-dial. if it all works out logically, the first case will NOT show any call end/call start for the person that left, and our own event will be 2 separate events, the first being the incoming or outgoing call that started it all and the second being our outgoing re-dial...
						
						// reset the statement so that it begins the search anew (otherwise it would carry on with its state since last time we used the statement)
						sqlite3_reset( pCallInfoStmt ); // NOTE: this will not clear any existing bindings. to unset bindings you use sqlite3_clear_bindings(). we won't bother with that and will instead simply re-bind the parameter below.

						// bind parameters
						// pCallInfoStmt = "SELECT duration FROM Calls WHERE (conv_dbid=? AND begin_timestamp<=?) ORDER BY begin_timestamp DESC LIMIT 1"
						sqlite3_bind_int( pCallInfoStmt, 1, thisEvent.row_convo_id ); // convoID is a 32 bit integer on all systems, so just bind it as-is using SQLite's 32 bit int bind function.
						sqlite3_bind_int64( pCallInfoStmt, 2, thisEvent.row_timestamp ); // always treat timestamp as a 64 bit value

						// grab the duration field if we've got a match
						int32_t callDuration = 0; // initialize duration to NULL (int 0) just in case we don't have a match. NOTE: we use a 32 bit duration because there is no way in HELL that anyone can ever have a call that overflows it.
						if( sqlite3_step( pCallInfoStmt ) == SQLITE_ROW ){
							// alright we've found the duration, so copy the value.
							callDuration = sqlite3_column_int( pCallInfoStmt, 0 ); // is NULL (int 0) on no answer (declined on either end/timed out), since the sqlite3_column_int() function returns 0 for null integers (even though the database value is technically non-existent)
						}

						// save the call state (answered/noanswer) and duration (if applicable)
						thisEvent.asXHTML << "<div class=\"callicons call_" << ( callDuration == 0 ? "noanswer" : "ended" ) << "\"><span></span></div> ";
						thisEvent.asXHTML << "Call ended" << ( callDuration == 0 ? ", no answer." : std::string( ". Duration: " ) + formatDuration( callDuration ) + "." ); // NOTE: we use the same "call ended" message for both conferences and regular calls. it looks good.
					}
					thisEvent.asXHTML << "</div>";
					break;
					} // end of type30 and type39 (call start/call end) parsing
				case 10: // person(s) added to conference; can be you or can be others
					if( !isConference ){ // regular chats
						// if we see this event and isConference is false then it MEANS *WE* created it, because that is the ONLY time a type10 event occurs with a dialog_partner of the person you're talking to.
						thisEvent.asXHTML << thisEvent.row_from_dispname << " created a separate conference \"<a href=\"#conf_" << thisEvent.row_convo_id << "\">" << getConferenceTitle( thisEvent.row_convo_id ) << "</a>\".";
					}else{ // conferences
						// thisEvent.row_author/thisEvent.row_from_dispname = the person that added others.
						thisEvent.asXHTML << thisEvent.row_from_dispname << " added ";

						// thisEvent.row_identities = space-separated list of everyone that was added (will only have more than 1 name when YOU created the conference with 2+ people at once (but not when others do that; in that case you get ONE entry in type10 which is your own name, and then a type100 with the other names), OR when someone adds 2+ people in one go while INSIDE the conference). // FIXME: actually, what happens if you are in a conference and a 3rd party adds 2 people at once? 2 separate events or 1?
						std::string identities = thisEvent.row_identities; // NOTE: we MUST do create this temporary variable because the boost::tokenizer constructor cannot accept a std::string() constructed as a function argument (attempting that will lead to the tokenizer replacing the first character of the first token with what appears to be a space character, instead of giving us the proper first token)
						boost::char_separator<char> sep( " " );
						boost::tokenizer< boost::char_separator<char> > tokens( identities, sep );
						for( boost::tokenizer< boost::char_separator<char> >::iterator identities_it( tokens.begin() ); identities_it != tokens.end(); ++identities_it ){
							std::string displayName = getDisplayNameAtTime( (*identities_it), thisEvent.row_timestamp ); // this grabs the person's name at the time they were invited, or the earliest name if we don't know their name yet.
							if( displayName == "" ){ displayName = (*identities_it); } // if no displayname was found, fall back to showing the raw SkypeID instead; this is what Skype does too if it doesn't know the details for a contact yet.
							thisEvent.asXHTML << displayName;
							if( boost::next( identities_it ) != tokens.end() ){ thisEvent.asXHTML << ", "; } // appended after every element except the last one
						}

						// all done
						thisEvent.asXHTML << " to the conference.";
					}
					break;
				case 13: // person left the conference; can be you or can be others
					// show the name of the person that left the conference; since this event only happens in conferences (not in regular chats), we don't need to check the "isConference" flag, and we can use the message direction to determine whether we left or if it was someone else (we could also check the author field for our skypeID but why bother? event direction tells us what we need to know).
					thisEvent.asXHTML << ( thisEvent.direction == 4 ? std::string( thisEvent.row_from_dispname) + " has" : "You" ) << " left the conference.";
					break;
				}


				// Output the message
				xhtmlOutput	<< "				<div class=\"E t" << thisEvent.row_type << "\">"
							<< "<div class=\"M\">" << thisEvent.asXHTML.str() << "</div>"; // the formatted message body for the event
				if( thisEvent.row_chatmsg_status == 1 ){ // if this is a pending (outgoing) message, show it as pending
				xhtmlOutput	<< "<div class=\"T\">Pending</div>";
				}else{ // otherwise show the original timestamp including edited/pending state (event timestamp is formatted as "8:05:07 AM")
				xhtmlOutput	<< "<div class=\"T\">" << ( thisEvent.row_edited_timestamp != 0 ? "<div class=\"icons msg_edit\"><span>Edited</span></div> " : "" ) << formatTime( &thisEvent.timestamp_tm, 1 ) << "</div>"; // row_edited_timestamp is NULL (int 0) when no edit has taken place, as the sqlite3_column_int64() function returns int 0 for NULL values
				}
				xhtmlOutput	<< "</div>\n";


				/************************* EVENT PARSING  END  *************************/
				++parserState.eventCount; // increment event counter
				break;
				}
			default:
				// FIXME: uncomment if you want to add support for the few missing events (notably type 100 for conference participant sync (Skype itself does not render that, though), and 50/51 for friend requests... really, everything of importance is fully supported already)
				//std::cerr << "Unsupported message type: " << thisEvent.row_type << "\n";
				//exit(1);
				break;
			} // end switch( thisEvent.row_type )
		} // end while() row-parsing
		if( parserState.chunkState != 0 ){ xhtmlOutput << "			</div>\n		</div>\n"; parserState.chunkState = 0; } // close the previous <div class="MC"> message-chunk container if one is open (NOTE: one will always be open at the end, so this is always true)
		xhtmlOutput	<< "	</div>\n"; // closes the final <div class="DayContainer">


		// Grab Messages: Cleanup
		// free up statement
		sqlite3_finalize( pStmt );


		// End!
		clock_end = clock();
		double clock_elapsed_sec = ( (double)( clock_end - clock_start ) ) / CLOCKS_PER_SEC;
		size_t clock_elapsed_usec = (size_t)( clock_elapsed_sec * 1000 ); // truncate to pure milliseconds without decimals
		if( clock_elapsed_usec < 1 ){ clock_elapsed_usec = 1; } // prevents nonsensical "Exported 10 events in 0 milliseconds." output, which sometimes occurs due to the limited resolution of the clock() cycle timer


		// Footer
		xhtmlOutput	<< "	<div class=\"HistoryFooter\">\n" // starts the history-footer which simply contains the generation time.
					<< "		<div class=\"LogFooterLine\">Exported " << parserState.eventCount << " events in " << clock_elapsed_usec << " millisecond" << ( clock_elapsed_usec != 1 ? "s" : "" ) << ".</div>\n" // the log generation runtime
					<< "	</div>\n" // closes the history-footer
					<< "</div>"; // closes the main <div class="ChatHistory"> log-container for this person


		// free up pre-compiled file transfer/call information statements
		sqlite3_finalize( pTransferInfoStmt );
		sqlite3_finalize( pCallInfoStmt );


		// all done! return the resulting XHTML structure.
		return xhtmlOutput.str();
	}
}
