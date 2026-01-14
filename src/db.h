#pragma once

struct db_conn;

struct db_conn *db_init();
void db_free(struct db_conn *conn);
int db_add_play(struct db_conn *conn, const char *title, const char *artist, const char *album, int mpd_song_id);

void db_free_results(char **results, int n_results);
int db_fetch_recent_songs(struct db_conn *db, char ***results);
int db_fetch_recent_artists(struct db_conn *db, char ***results);
int db_fetch_recent_albums(struct db_conn *db, char ***results);
int db_fetch_frequent_songs(struct db_conn *db, char ***results);
int db_fetch_frequent_artists(struct db_conn *db, char ***results);
int db_fetch_frequent_albums(struct db_conn *db, char ***results);

