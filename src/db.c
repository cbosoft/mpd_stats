#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "db.h"

struct db_conn {
  sqlite3 *inner;
};


int init_schema(sqlite3 *db) {
  const char *statements[] = {
    "CREATE TABLE IF NOT EXISTS Artist(ID INTEGER PRIMARY KEY, Name TEXT NOT NULL UNIQUE);",
    "CREATE TABLE IF NOT EXISTS Album(ID INTEGER PRIMARY KEY, ArtistID INTEGER NOT NULL REFERENCES Artist(ID), Name TEXT NOT NULL, UNIQUE (ArtistID, Name), CHECK (ArtistID > 0) );",
    "CREATE TABLE IF NOT EXISTS Song(ID INTEGER PRIMARY KEY, AlbumID INTEGER NOT NULL REFERENCES Album(ID), Name TEXT NOT NULL, MPDID INTEGER NOT NULL UNIQUE, UNIQUE (AlbumID, Name), Check(AlbumID > 0));",
    "CREATE TABLE IF NOT EXISTS Plays(Time TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, SongID INTEGER REFERENCES Song(ID));",
    NULL
  };
  char *errmsg = NULL;
  int rv = 0;

  for (int i = 0; statements[i] != NULL; i++) {
    const char *sql = statements[i];
    if (sqlite3_exec(db, sql, NULL, NULL, (char **)&errmsg)) {
      fprintf(stderr, "Error running sql \"%s\": %s\n", sql, errmsg);
      rv = -1;
      break;
    }
  }

  return rv;
}


struct db_conn *db_init() {
  struct db_conn *conn = malloc(sizeof(struct db_conn));
  conn->inner = NULL;

  const char *HOME = getenv("HOME");
  if (HOME == NULL) {
    fprintf(stderr, "No HOME var?");
    goto db_init_err;
  }

  char s[1000] = {0};
  snprintf(s, 1000, "%s/.mpd_stats.db", HOME);

  if (sqlite3_open((const char *)s, &conn->inner)) {
    fprintf(stderr, "Failed to open sqlite db.\n");
    goto db_init_err;
  }

  if (init_schema(conn->inner)) {
    fprintf(stderr, "Failed to init sqlite schema.\n");
    goto db_init_err;
  }

  return conn;

db_init_err:
  db_free(conn);
  return NULL;
}


void db_free(struct db_conn *db) {
  if (db == NULL) return;

  if (db->inner != NULL) {
    sqlite3_close(db-> inner);
  }

  free(db);
  return;
}


int db_add_artist(sqlite3 *db, const char *artist) {
  const char *sql = "INSERT INTO Artist(Name) VALUES (?) RETURNING ID;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep select stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_text(stmt, 1, artist, strlen(artist), SQLITE_STATIC)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:Name to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_artist_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      rv = sqlite3_column_int(stmt, 0);
      break;
    default:
      rv = -1;
      break;
  }

_db_add_artist_end:
  sqlite3_finalize(stmt);
  return rv;
}


int db_get_artist(sqlite3 *db, const char *artist) {
  const char *sql = "SELECT ID FROM Artist WHERE Name=?;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep select stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_text(stmt, 1, artist, strlen(artist), SQLITE_STATIC)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:Name to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_get_artist_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      // found artist
      rv = sqlite3_column_int(stmt, 0);
      break;
    case SQLITE_DONE:
      // no data :(
      rv = db_add_artist(db, artist);
      break;
    default:
      rv = -1;
      break;
  }

_db_get_artist_end:
  sqlite3_finalize(stmt);
  return rv;
}


int db_add_album(sqlite3 *db, const char *album, int artist_id) {
  const char *sql = "INSERT INTO Album (Name, ArtistID) VALUES (?, ?) RETURNING ID;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep select stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_text(stmt, 1, album, strlen(album), SQLITE_STATIC)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:Album to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_album_end;
  }

  if (sqlite3_bind_int(stmt, 2, artist_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 2:ArtistID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_album_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      rv = sqlite3_column_int(stmt, 0);
      break;
    default:
      rv = -1;
      break;
  }

_db_add_album_end:
  sqlite3_finalize(stmt);
  return rv;
}


int db_get_album(sqlite3 *db, const char *album, int artist_id) {
  const char *sql = "SELECT ID FROM Album WHERE Name=? AND ArtistID=?;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep select stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_text(stmt, 1, album, strlen(album), SQLITE_STATIC)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:Name to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_get_album_end;
  }

  if (sqlite3_bind_int(stmt, 2, artist_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 2:ArtistID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_get_album_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      // found album
      rv = sqlite3_column_int(stmt, 0);
      break;
    case SQLITE_DONE:
      // no data :(
      rv = db_add_album(db, album, artist_id);
      break;
    default:
      rv = -1;
      break;
  }

_db_get_album_end:
  sqlite3_finalize(stmt);
  return rv;
}



int db_add_song(sqlite3 *db, const char *title, int album_id, int mpd_song_id) {
  const char *sql = "INSERT INTO Song (Name, AlbumID, MPDID) VALUES (?, ?, ?) RETURNING ID;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_text(stmt, 1, title, strlen(title), SQLITE_STATIC)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:Name to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_song_end;
  }

  if (sqlite3_bind_int(stmt, 2, album_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 2:AlbumID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_song_end;
  }

  if (sqlite3_bind_int(stmt, 3, mpd_song_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 3:MPDID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_song_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      // found album
      rv = sqlite3_column_int(stmt, 0);
      break;
    default:
      rv = -1;
      break;
  }

_db_add_song_end:
  sqlite3_finalize(stmt);
  return rv;
}

int db_get_song(sqlite3 *db, const char *title, int album_id, int mpd_song_id) {
  const char *sql = "SELECT ID FROM Song WHERE MPDID=?;";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_int(stmt, 1, mpd_song_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:MPDID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_get_song_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      // found song
      rv = sqlite3_column_int(stmt, 0);
      break;
    case SQLITE_DONE:
      // no data :(
      rv = db_add_song(db, title, album_id, mpd_song_id);
      break;
    default:
      rv = -1;
      break;
  }

_db_get_song_end:
  sqlite3_finalize(stmt);
  return rv;
}


int _db_add_play(sqlite3 *db, int song_id) {
  const char *sql = "INSERT INTO Plays (SongID) VALUES (?);";
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep stmt \"%s\": %s\n", sql, errmsg);
    return -1;
  }

  int rv = -1;
  if (sqlite3_bind_int(stmt, 1, song_id)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to bind var 1:SongID to stmt \"%s\": %s\n", sql, errmsg);
    goto _db_add_song_end;
  }

  switch (sqlite3_step(stmt)) {
    case SQLITE_DONE:
      rv = 0;
      break;
    default:
      rv = -1;
      break;
  }

_db_add_song_end:
  sqlite3_finalize(stmt);
  return rv;
}


int db_add_play(struct db_conn *db, const char *title, const char *artist, const char *album, int mpd_song_id) {
  int song_id = db_get_song(db->inner, title, -1, mpd_song_id);

  if (song_id < 0) {
    int artist_id = db_get_artist(db->inner, artist);
    if (artist_id < 0) { return -1; }

    int album_id = db_get_album(db->inner, album, artist_id);
    if (album_id < 0) { return -1; }

    song_id = db_add_song(db->inner, title, album_id, mpd_song_id);
  }

  return _db_add_play(db->inner, song_id);
}



// Querying

const char *SQL_RECENT_ARTISTS = "SELECT ArtistName FROM (SELECT Artist.Name AS ArtistName, Max(Time) AS Time FROM Plays INNER JOIN Song s ON s.id=plays.SongID INNER JOIN Album ON s.AlbumID = Album.ID INNER JOIN Artist ON Album.ArtistID=Artist.ID GROUP BY Artist.Name) ORDER BY Time DESC LIMIT 10;";
const char *SQL_RECENT_ALBUMS = "SELECT AlbumName FROM (SELECT Album.Name AS AlbumName, Max(Time) AS Time FROM Plays INNER JOIN Song s ON s.id=plays.SongID INNER JOIN Album ON s.AlbumID = Album.ID GROUP BY Album.Name) ORDER BY Time DESC LIMIT 10;";
const char *SQL_RECENT_SONGS = "SELECT SongName FROM (SELECT S.Name AS SongName, MAX(Time) AS Time FROM Plays INNER JOIN Song s ON s.id=plays.SongID GROUP BY S.Name) ORDER BY Time DESC LIMIT 10;";

const char *SQL_FREQUENT_ARTISTS = "SELECT ArtistName FROM (SELECT Artist.Name as ArtistName, COUNT(*) as PlayCount FROM Plays INNER JOIN Song s ON s.id=plays.SongID INNER JOIN Album ON s.AlbumID = Album.ID INNER JOIN Artist ON Album.ArtistID=Artist.ID GROUP BY Artist.Name) ORDER BY PlayCount Desc LIMIT 10;";
const char *SQL_FREQUENT_ALBUMS = "SELECT AlbumName FROM (SELECT Album.Name as AlbumName, COUNT(*) as PlayCount FROM Plays INNER JOIN Song s ON s.id=plays.SongID INNER JOIN Album ON s.AlbumID = Album.ID GROUP BY Album.Name) ORDER BY PlayCount Desc LIMIT 10;";
const char *SQL_FREQUENT_SONGS = "SELECT SongName FROM (SELECT S.Name as SongName, COUNT(*) as PlayCount FROM Plays INNER JOIN Song s ON s.id=plays.SongID GROUP BY S.Name) ORDER BY PlayCount Desc LIMIT 100;";


int fetch_results(sqlite3 *db, const char *query, char ***results) {
  sqlite3_stmt *stmt = NULL;
  if (sqlite3_prepare_v2(db, query, strlen(query), &stmt, NULL)) {
    const char *errmsg = sqlite3_errmsg(db);
    fprintf(stderr, "failed to prep stmt \"%s\": %s\n", query, errmsg);
    return 0;
  }

  int count = 0;
  (*results) = malloc(10*sizeof(const char *));
  for (int state = sqlite3_step(stmt); state == SQLITE_ROW; state=sqlite3_step(stmt)) {
    const char *s = (const char *)sqlite3_column_text(stmt, 0);
    int n = strlen(s);
    (*results)[count] = malloc(sizeof(char)*(n + 1));
    strncpy((*results)[count], s, n);
    (*results)[count][n] = 0;
    count ++;
  }

  sqlite3_finalize(stmt);
  return count;
}

int db_fetch_recent_artists(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_RECENT_ARTISTS, results); }
int db_fetch_recent_albums(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_RECENT_ALBUMS, results); }
int db_fetch_recent_songs(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_RECENT_SONGS, results); }
int db_fetch_frequent_artists(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_FREQUENT_ARTISTS, results); }
int db_fetch_frequent_albums(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_FREQUENT_ALBUMS, results); }
int db_fetch_frequent_songs(struct db_conn *db, char ***results) { return fetch_results(db->inner, SQL_FREQUENT_SONGS, results); }

void db_free_results(char **results, int n_results) {
  for (int i = 0; i < n_results; i++) {
    free(results[i]);
  }
  free(results);
}
