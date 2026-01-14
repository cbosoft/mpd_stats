#pragma once

struct db_conn;

struct db_conn *db_init();
void db_free(struct db_conn *conn);
int db_add_play(struct db_conn *conn, const char *title, const char *artist, const char *album, int mpd_song_id);

