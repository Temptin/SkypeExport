#include "skypeparser.h"

namespace SkypeParser
{
	CSkypeParser::CSkypeParser( const std::string &dbPath )
	{
		int rc;

		// store database path
		mDBPath = dbPath;

		// open skype history database
		rc = sqlite3_open_v2( mDBPath.c_str(), &mDB, SQLITE_OPEN_READONLY, NULL );
		if( rc != SQLITE_OK ){
			// FIXME: we do not have to call sqlite3_close( mDB ); here, since the open command failed. doing so is nonsensical and causes "library routine called out of sequence". however, we might want to check the return code in case there's SOME situation where we still need to close the database.
			throw std::runtime_error( sqlite3_errmsg( mDB ) );
		}

		// prepare the internal data
		try{
			buildInternalData();
		}catch( const std::exception ){
			sqlite3_close( mDB );
			throw;
		}
	}

	CSkypeParser::~CSkypeParser()
	{
		// release the database lock
		sqlite3_close( mDB );
	}


	void CSkypeParser::buildInternalData()
	{
		int rc;
		sqlite3_stmt *pStmt;

		/* My Skype ID */

		// prepare statement
		rc = sqlite3_prepare_v2( mDB, "SELECT skypename FROM Accounts ORDER BY id ASC LIMIT 1", -1, &pStmt, NULL ); // FIXME: is there really only 1 account per database? skype makes a new folder and new main.db for each user so it seems that way, yes.
		if( rc != SQLITE_OK ){
			sqlite3_finalize( pStmt );
			throw std::runtime_error( sqlite3_errmsg( mDB ) );
		}

		// grab the account id
		if( sqlite3_step( pStmt ) == SQLITE_ROW ){
			// grab column in result row
			mMySkypeID = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 0 ) ); // skypename
		}else{
			sqlite3_finalize( pStmt );
			throw std::runtime_error( "unable to locate Skype account ID in database" );
		}

		// free up statement
		sqlite3_finalize( pStmt );


		/* Users */

		// prepare statement (NOTE: the reason we don't use the Contacts table is that it DOESN'T contain people you have only spoken to in a conference via a mutual friend, DOESN'T ALWAYS contain deleted friends (who you may want to be able to export), and yet it DOES contain useless spam-contacts and people you just saw in people-search, as well as people you've added but never talked to (which would be pointless to output). sure, there's the buddystatus field, but that still doesn't ensure that it's someone you've actually had a single conversation with. therefore, the below statement is far better, since it truly grabs everyone you've had contact with at least once.)
		// NOTE: the "type!=50" check ensures that we IGNORE all "please add me as a friend" events, since those are very often sent by spammers. you of course dismiss their dialog, but they still get saved in the Contacts table and a "type 50" event is logged with their name in the Messages table, which is why we must avoid "friend request" events! our scanning does not need them, since it's very thorough and ensures that anybody who has sent (or received) a single non-friend request event from/to you will be detected!
		// NOTE: we do the "IS NOT NULL" and "!=''" (not empty string) checks to protect against a corrupt Skype database with NULL/empty author in some messages (and the other column, dialog_partner, is very often naturally NULL in various events). the NULL/empty string corruption of "author" is very rare, but happens due to the awful "cloud history" service from Skype, which sometimes NULLs/clears (empty string) critical fields like "author".
		// NOTE: the "NOT GLOB" check avoids all names with # or $ or @ in their IDs, since those are illegal characters in *real* SkypeIDs and are only used internally for certain situations that seem related to "cloud history" data not having finished downloading yet (examples of those internal IDs are "19:0062db31a2714735a93f5a8365cb0838@thread.skype" and "#test.acc2001/$test.acc2004;8c67a909bc2228f8"; I've only seen the prior format and only in a single row in the dialog_partner field for a partial "cloud history" test database, and it vanished after the database continued downloading the history, but we might as well validate all names to make sure no temporary, internal names are caught anywhere)
		rc = sqlite3_prepare_v2( mDB,
			"SELECT DISTINCT author AS skypename FROM Messages WHERE (type!=50 AND author IS NOT NULL AND author!='' AND author NOT GLOB '*[#$@]*') " // finds non-duplicate people who have sent at least one non-friend-request event TO you; it also finds yourself (since you're the author of your own events), but we ignore your own name when we actually parse these results
			"UNION " // the UNION joins the results of two SELECT statements and makes sure there are no duplicate rows (whereas UNION ALL would allow duplicates)
			"SELECT DISTINCT dialog_partner AS skypename FROM Messages WHERE (type!=50 AND dialog_partner IS NOT NULL AND dialog_partner!='' AND dialog_partner NOT GLOB '*[#$@]*') " // in case the other person never wrote back to you, this additional statement finds the remaining non-duplicate people YOU have sent at least one non-friend-request event to
			"ORDER BY skypename ASC", // ensures that our list will be sorted alphabetically
			-1, &pStmt, NULL );
		if( rc != SQLITE_OK ){
			sqlite3_finalize( pStmt );
			throw std::runtime_error( sqlite3_errmsg( mDB ) );
		}

		// populate user list
		skypeIDs_t::iterator skypeUsers_it;
		while( sqlite3_step( pStmt ) == SQLITE_ROW ){
			// grab columns in current row
			std::string skypeID = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 0 ) ); // the "skypename" column we invented, which contains all unique non-null author and dialog_partner values

			// if this skypeID is actually ourselves (the database owner), then don't add it, since this is only meant to hold our contacts
			if( skypeID == mMySkypeID ){ continue; }

			// insert this skype user if it's the first time we encounter it (which it will be as the SQL query is DISTINCT)
			skypeUsers_it = std::find( mSkypeUsers.begin(), mSkypeUsers.end(), skypeID );
			if( skypeUsers_it == mSkypeUsers.end() ){ // first time we encounter this user; store them in the list
				mSkypeUsers.push_back( skypeID );
			}
		}

		// free up statement
		sqlite3_finalize( pStmt );


		/* Conferences: Phase 1 (IDs and Titles) */

		// prepare statement to get the id and title of all conferences
		rc = sqlite3_prepare_v2( mDB, "SELECT id, displayname FROM Conversations WHERE (type=2)", -1, &pStmt, NULL );
		if( rc != SQLITE_OK ){
			sqlite3_finalize( pStmt );
			throw std::runtime_error( sqlite3_errmsg( mDB ) );
		}

		// iterate through all conferences and store their ids and titles (we will add the participant skypeIDs in the next step)
		// NOTE: if the user has never been in any conferences, this won't execute and no conferences will be added to mSkypeConferences
		skypeConferences_t::iterator skypeConferences_it;
		while( sqlite3_step( pStmt ) == SQLITE_ROW ){
			// grab columns in current row
			int32_t convoID = sqlite3_column_int( pStmt, 0 ); // id
			const char *pDisplayName = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 1 ) );
			std::string displayName = ( pDisplayName != NULL ? pDisplayName : "" ); // displayname ("conference title"), has been seen as NULL for some WEIRD conferences with ZERO "messages" and just myself as participant, weird as hell... there's also the weird "Mood Messages" conference with just yourself as participant. seems Skype's protocol designers stupidly used conferences for some internal data stuff too.

			// insert this conference ID if we haven't found it before (which we shouldn't have, as all IDs are unique)
			skypeConferences_it = mSkypeConferences.find( convoID );
			if( skypeConferences_it == mSkypeConferences.end() ){ // first time we encounter this conference; set up initial variables
				struct ConferenceInfo confInfo;
				confInfo.confTitle = displayName; // the name of the conference minus yourself. some names may sometimes be represented as "..." (might happen for people that aren't on your contact list, not sure how Skype determines it, or maybe it happens for conferences you did not create)
				mSkypeConferences.insert( std::pair<int32_t,struct ConferenceInfo>( convoID, confInfo ) );
			}
		}

		// free up statement
		sqlite3_finalize( pStmt );


		/* Conferences: Phase 2 (Participants) */

		// only perform this step if the conference scan actually found any conferences in the database
		// NOTE: this extra verification fixes the error where people who have never been in any conferences got an error saying "syntax error near ")"", since it was building a "WHERE ()" statement without any conditions in that case. besides, there's no participants to look up if they haven't got any conferences, so no need to run this code for those users! ;-)
		if( !mSkypeConferences.empty() ){
			// construct a statement containing the IDs for every conference we've discovered (the Participants table contains an accurate, clean list of every skypeID that was ever added to that conference, no matter how they were added and whether they said something or not, so it's perfect for our needs)
			std::stringstream confParticipantQuery( std::stringstream::in | std::stringstream::out );
			confParticipantQuery << "SELECT DISTINCT convo_id, identity FROM Participants WHERE ("; // the DISTINCT is there for safety; I've never seen duplicates but don't wanna risk it.
			for( skypeConferences_t::const_iterator it( mSkypeConferences.begin() ); it != mSkypeConferences.end(); ++it ){
				confParticipantQuery << "convo_id='" << (*it).first << "'";
				if( boost::next( it ) != mSkypeConferences.end() ){ confParticipantQuery << " OR "; } // appended after every element except the last one
			}
			confParticipantQuery << ")";

			// prepare statement to get the participants for all encountered conferences
			rc = sqlite3_prepare_v2( mDB, confParticipantQuery.str().c_str(), -1, &pStmt, NULL );
			if( rc != SQLITE_OK ){
				sqlite3_finalize( pStmt );
				throw std::runtime_error( sqlite3_errmsg( mDB ) );
			}

			// iterate through all participants and store them under their given conferences
			while( sqlite3_step( pStmt ) == SQLITE_ROW ){
				// grab columns in current row
				int32_t convoID = sqlite3_column_int( pStmt, 0 ); // convo_id
				std::string participantSkypeID = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 1 ) ); // identity

				// look up the conference and insert this participant
				skypeConferences_it = mSkypeConferences.find( convoID );
				if( skypeConferences_it != mSkypeConferences.end() ){ // conference known
					(*skypeConferences_it).second.participants.push_back( participantSkypeID );
				}
			}

			// free up statement
			sqlite3_finalize( pStmt );
		}


		// all initializations ok!
	}


	const std::string& CSkypeParser::getMySkypeID()
	{
		return mMySkypeID;
	}


	const skypeIDs_t& CSkypeParser::getSkypeUsers()
	{
		return mSkypeUsers;
	}


	std::string CSkypeParser::getDisplayNameAtTime( const std::string &skypeID, time_t timestamp, bool allowFallback ) // NOTE: allowFallback is true by default in the header declaration, since it's almost always the desired behavior
	{
		int rc;
		sqlite3_stmt *pStmt;
		std::string nameQuery;
		std::string dispName = ( allowFallback ? skypeID : "" ); // if lookup fails this returns raw SkypeID if fallback allowed, otherwise ""
		bool foundName = false;

		// builds the appropriate query based on what we're trying to look up
		// NOTE: we do the "IS NOT NULL" check to protect against the increasingly common NULL corruption in Skype's databases
		if( timestamp == 0 ){ // earliest name
			nameQuery = "SELECT from_dispname FROM Messages WHERE (author=? AND from_dispname IS NOT NULL) ORDER BY timestamp ASC LIMIT 1";
		}else if( timestamp == -1 ){ // latest name
			nameQuery = "SELECT from_dispname FROM Messages WHERE (author=? AND from_dispname IS NOT NULL) ORDER BY timestamp DESC LIMIT 1";
		}else{ // name at specific point in time, and allows you to overshoot their latest timestamp and still grab their correct name at that time (NOTE: if this fails, we will return the earliest name instead, if possible, since it failing means that the earliest name is the next in sequence; this will fail quite a lot at the beginning of conferences where unknown people are invited via "identities")
			nameQuery = "SELECT from_dispname FROM Messages WHERE (author=? AND from_dispname IS NOT NULL AND timestamp<=?) ORDER BY timestamp DESC LIMIT 1";
		}

		// prepare statement
		rc = sqlite3_prepare_v2( mDB, nameQuery.c_str(), -1, &pStmt, NULL );
		if( rc != SQLITE_OK ){
			sqlite3_finalize( pStmt );
			return dispName; // sql failed to execute; returns fallback if allowed, otherwise ""
		}

		// bind parameters
		sqlite3_bind_text( pStmt, 1, skypeID.c_str(), -1, SQLITE_TRANSIENT );
		if( timestamp > 0 ){ // a specific timestamp was provided, bind it too
			sqlite3_bind_int64( pStmt, 2, timestamp ); // always treat timestamp as a 64 bit value
		}

		// grab the displayname, if found
		if( sqlite3_step( pStmt ) == SQLITE_ROW ){ // found
			// grab column in result row
			dispName = reinterpret_cast<const char *>( sqlite3_column_text( pStmt, 0 ) ); // from_dispname
			foundName = true;
		}

		// free up statement
		sqlite3_finalize( pStmt );

		// if no name was found and timestamp is > 0, it means a specific timestamp was required. in that case, we should re-try the query using "earliest name" instead.
		if( !foundName && timestamp > 0 ){ // no match, and we had asked for a specific timestamp
			return getDisplayNameAtTime( skypeID, 0, allowFallback ); // does a single re-query with "earliest name" method instead, which is guaranteed to solve cases where we're asking about their name slightly before their name appears in the log; will use fallback as a last resort if allowed
		}else{ // query succeeded, or query failed but was not of "timestamp > 0" type
			// returns their display name if found, otherwise raw SkypeID fallback if allowed or "" if not allowed
			return dispName;
		}

		// FIXME: if ALL of this fails, we MAY want to do one final "Contacts" table lookup instead, as that is guaranteed to have names for people that have joined conferences but caused 0 events and never been seen by us anywhere else; but then again, such people don't matter in the export since they contributed nothing, and the "raw SkypeID" fallback is good enough for them
	}


	std::string CSkypeParser::getConferenceTitle( int32_t convoID )
	{
		std::string conferenceTitle = "";

		skypeConferences_t::iterator skypeConferences_it( mSkypeConferences.find( convoID ) );
		if( skypeConferences_it != mSkypeConferences.end() ){ // conference known
			conferenceTitle = (*skypeConferences_it).second.confTitle;
		}

		return conferenceTitle; // "" on failure
	}


	std::vector<int32_t> CSkypeParser::getConferencesForSkypeID( const std::string &skypeID )
	{
		std::vector<int32_t> conferenceIDs;

		for( skypeConferences_t::const_iterator it( mSkypeConferences.begin() ); it != mSkypeConferences.end(); ++it ){ // conferences
			for( skypeIDs_t::const_iterator it2( (*it).second.participants.begin() ); it2 != (*it).second.participants.end(); ++it2 ){ // participants
				if( (*it2) == skypeID ){ // the skypeID took part in this conference
					conferenceIDs.push_back( (*it).first ); // add the conference ID
					break; // no need to iterate through further participants since we found what we're looking for
				}
			}
		}

		return conferenceIDs;
	}
}
