PRAGMA writable_schema = 1;
delete from sqlite_master where type in ('table', 'index', 'trigger');
PRAGMA writable_schema = 0;
VACUUM;
PRAGMA INTEGRITY_CHECK;
CREATE TABLE "users" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"name"		TEXT NOT NULL UNIQUE,
	"password"	TEXT NOT NULL,
	"email"		TEXT,
	"sid"		TEXT NOT NULL,
	"session"	TEXT NOT NULL
);
CREATE TABLE "resources" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"url"		TEXT NOT NULL,
	"hash"		TEXT NOT NULL,
	"userId"	INTEGER NOT NULL
);
CREATE TABLE "comments" (
	"id"		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	"headline"	TEXT NOT NULL,
	"content"	TEXT NOT NULL,
	"author"	TEXT NOT NULL,
	"votes"		TEXT,
	"date"		TEXT,
	"url"		TEXT
);
INSERT INTO "users" VALUES (1,'admin','code','root@commentsense.de','','');
INSERT INTO "users" VALUES (2,'test','test','test@commentsense.de','5264273458d4df8ed8d22a8d06e464a8af5d572388544c993af0597f700fcc1a','test|31.01.2020 16:56:45|23510|0|388153295');
INSERT INTO "comments" VALUES (1,'Commentsense','A browser plugin for leaving dumb comments on every website','admin','', DATETIME('now'),'');