#ifndef __SKYPEPARSER_H__
#define __SKYPEPARSER_H__

#ifdef _MSC_VER
// disables MSVC's attempts to get us to use their non-portable localtime_s and such instead.
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <list>
#include <boost/filesystem.hpp> // provides cross-platform path handling
namespace fs = boost::filesystem3;
#include <time.h> // provides timestamp conversion and clock timing
#include <stdint.h>
#include <cstring> // provides memcpy()
#include "../libs/sqlite3/sqlite3.h"
#include <boost/utility.hpp> // provides iterator next() which allows us to peek at the next iterator in sequence
#include <boost/tokenizer.hpp> // provides a way to access tokens in a string
#include <boost/regex.hpp> // SERIOUS FIXME: Replace message-string handling with the IBM ICU string class, and switch to using boost::u32regex. This adds UTF8, 16 and 32 regex support, which is needed to properly export logs making use of unicode characters.

// this is the size in bytes of a Skype GUID (used as value for certain database columns)
#define SKYPEPARSER_GUID_SIZE 32

namespace SkypeParser
{
	// container for holding a collection of skypeIDs
	typedef std::list<std::string> skypeIDs_t;
	
	// struct for holding conferences, their titles, and all the skypeids that took part in each
	struct ConferenceInfo {
		std::string confTitle;
		skypeIDs_t participants;
	};
	typedef std::map<int32_t,struct ConferenceInfo> skypeConferences_t;

	// ...
	class CSkypeParser
	{
	private:
		std::string mDBPath;
		sqlite3 *mDB;

		std::string mMySkypeID;
		skypeIDs_t mSkypeUsers;
		skypeConferences_t mSkypeConferences;
		void buildInternalData();

		std::string formatTime( const struct tm *timestamp_tm, uint8_t format );
		std::string formatBytes( uint64_t bytes, bool allowZeroTrail );
		std::string formatDuration( uint32_t duration_sec );
		std::string skypeMessageToXHTML( const char *msgText );

	public:
		CSkypeParser( const std::string &dbPath );
		~CSkypeParser();

		const std::string& getMySkypeID();
		const skypeIDs_t& getSkypeUsers();
		std::string getDisplayNameAtTime( const std::string &skypeID, time_t timestamp );

		std::string getConferenceTitle( int32_t convoID );
		std::vector<int32_t> getConferencesForSkypeID( const std::string &skypeID );

		void exportUserHistory( const std::string &skypeID, const std::string &targetFile );
		std::string getHistoryAsXHTML( void *searchValue, bool isConference );
	};
}

#endif