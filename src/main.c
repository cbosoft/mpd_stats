#include <stdlib.h>
#include <stdio.h>

// docs: https://www.musicpd.org/doc/libmpdclient
#include <mpd/client.h>

#include "msleep.h"
#include "db.h"
#include "playlist.h"


struct stat_state {
  int song_id;
  int new_song;
};


void stat_state_update(struct stat_state *state, struct mpd_connection *conn) {
    struct mpd_status *status = mpd_run_status(conn);

    int song_id = mpd_status_get_song_id(status);
    if (song_id < 0) {
      state->new_song = 0;
    }
    else if (song_id != state->song_id) {
      state->new_song = 1;
      state->song_id = song_id;
    }
    else {
      state->new_song = 0;
    }

    mpd_status_free(status);
}

void run_sm(struct stat_state *state, struct mpd_connection *mpd, struct db_conn *db) {
  if (state->new_song) {
    // get song info
    // store in DB
    struct mpd_song *song = mpd_run_current_song(mpd);
    int song_id = mpd_song_get_id(song);
    const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    const char *album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    fprintf(stderr, "now playing %s by %s from %s\n", title, artist, album);
    if (db_add_play(db, title, artist, album, song_id)) {
      fprintf(stderr, "failed to add to DB!\n");
    }

    mpd_song_free(song);

    generate_playlists(mpd, db);
  }
}


int mpd_error(struct mpd_connection *mpd) {
  fprintf(stderr, "MPD error: %s\n", mpd_connection_get_error_message(mpd));
  mpd_connection_free(mpd);
  return 1;
}


int main() {
  struct mpd_connection *mpd = mpd_connection_new(NULL, 0, 0);
  //if (!mpd_search_db_tags(mpd, MPD_TAG_ARTIST)) return mpd_error(mpd);
  // if (!mpd_search_add_db_songs_to_playlist(mpd, "foo")) return mpd_error(mpd);
  // if (!mpd_search_add_tag_constraint(mpd, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, "rammstein")) return mpd_error(mpd);
  // //if (!mpd_search_add_expression(mpd, "rammstein")) return mpd_error(mpd);
  // if (!mpd_search_commit(mpd)) return mpd_error(mpd);
  // mpd_connection_free(mpd);
  // return 1;

  if (mpd == NULL) {
    fprintf(stderr, "Out of memory\n");
    return 1;
  }

  if (mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS) {
    fprintf(stderr, "MPD connection failed: %s\n", mpd_connection_get_error_message(mpd));
    mpd_connection_free(mpd);
    return 2;
  }

  struct db_conn *db = db_init();
  if (db == NULL) {
    fprintf(stderr, "DB init failed\n");
    mpd_connection_free(mpd);
    return 3;
  }

  struct stat_state state = {.song_id = 0, .new_song = 0};
  while (1) {
    stat_state_update(&state, mpd);
    run_sm(&state, mpd, db);
    msleep(500);
  }

  mpd_connection_free(mpd);
  db_free(db);
  return 0;
}
