CREATE TABLE
IF
	NOT EXISTS musics (
		musicId integer PRIMARY KEY AUTOINCREMENT,
		track integer,
		title TEXT,
		path TEXT NOT NULL,
		parentPath TEXT NOT NULL,
		offset DOUBLE DEFAULT 0.0,
		duration DOUBLE,
		durationStr TEXT,
		fileName TEXT,
		fileExt TEXT,
		bitRate integer,
		sampleRate integer,
		rating integer,
		dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
		albumReplayGain DOUBLE DEFAULT NULL,
		albumPeak DOUBLE DEFAULT NULL,
		trackReplayGain DOUBLE DEFAULT NULL,
		trackPeak DOUBLE DEFAULT NULL,
		genre TEXT,
		comment TEXT,
		fileSize integer,
		heart integer,
		plays integer DEFAULT 0,
		lyrc TEXT,
		trLyrc TEXT,
		coverId TEXT DEFAULT NULL,
		UNIQUE ( path, offset, durationStr ) 
	);
	
CREATE UNIQUE INDEX
IF
	NOT EXISTS path_index ON musics ( path, offset, durationStr );
	
CREATE TABLE
IF
	NOT EXISTS playlist ( playlistId integer PRIMARY KEY AUTOINCREMENT, playlistIndex integer, storeType integer, cloudPlaylistId TEXT, name TEXT NOT NULL );

CREATE INDEX IF NOT EXISTS playlist_name_index ON playlist (name);
	
CREATE TABLE
IF
	NOT EXISTS albums (
		albumId integer PRIMARY KEY AUTOINCREMENT,
		artistId integer,
		album TEXT NOT NULL DEFAULT '',
		coverId TEXT,
		discId TEXT,
		dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
		year integer,
		heart integer,
		storeType integer,		
		isHiRes integer,
		isSelected integer DEFAULT 0,
		plays integer DEFAULT 0,
		FOREIGN KEY ( artistId ) REFERENCES artists ( artistId ),
		UNIQUE ( albumId, artistId ) 
	);

CREATE INDEX IF NOT EXISTS album_name_index ON albums (album);
	
CREATE TABLE
IF
	NOT EXISTS artists (
		artistId integer PRIMARY KEY AUTOINCREMENT,
		artist TEXT NOT NULL DEFAULT '',
		artistNameEn TEXT NOT NULL DEFAULT '',
		coverId TEXT,
		firstChar TEXT,
		firstCharEn TEXT,
		dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP 
	);

CREATE INDEX IF NOT EXISTS artist_name_index ON artists (artist);
	
CREATE TABLE
IF
	NOT EXISTS albumArtist (
		albumArtistId integer PRIMARY KEY AUTOINCREMENT,
		artistId integer,
		albumId integer,
		FOREIGN KEY ( artistId ) REFERENCES artists ( artistId ),
		FOREIGN KEY ( albumId ) REFERENCES albums ( albumId ) 
	);
	
CREATE TABLE
IF
	NOT EXISTS albumCategories ( albumCategoryId integer PRIMARY KEY AUTOINCREMENT, category TEXT, albumId integer, FOREIGN KEY ( albumId ) REFERENCES albums ( albumId ) );
CREATE TABLE
IF
	NOT EXISTS albumMusic (
		albumMusicId integer PRIMARY KEY AUTOINCREMENT,
		musicId integer,
		artistId integer,
		albumId integer,
		FOREIGN KEY ( musicId ) REFERENCES musics ( musicId ),
		FOREIGN KEY ( artistId ) REFERENCES artists ( artistId ),
		FOREIGN KEY ( albumId ) REFERENCES albums ( albumId ),
		UNIQUE ( musicId ) 
	);
	
CREATE TABLE
IF
	NOT EXISTS musicLoudness (
		musicLoudnessId integer PRIMARY KEY AUTOINCREMENT,
		musicId integer,
		artistId integer,
		albumId integer,
		trackLoudness DOUBLE,
		FOREIGN KEY ( musicId ) REFERENCES musics ( musicId ),
		FOREIGN KEY ( artistId ) REFERENCES artists ( artistId ),
		FOREIGN KEY ( albumId ) REFERENCES albums ( albumId ),
		UNIQUE ( musicId ) 
	);
	
CREATE TABLE
IF
	NOT EXISTS playlistMusics (
		playlistMusicsId integer PRIMARY KEY AUTOINCREMENT,
		playlistId integer,
		musicId integer,
		albumId integer,
		playing integer,
		isChecked integer,
		FOREIGN KEY ( playlistId ) REFERENCES playlist ( playlistId ),
		FOREIGN KEY ( musicId ) REFERENCES musics ( musicId ),
	    FOREIGN KEY ( albumId ) REFERENCES albums ( albumId ) 
	);