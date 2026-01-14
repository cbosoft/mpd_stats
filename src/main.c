#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

// docs: https://www.musicpd.org/doc/libmpdclient
#include <mpd/client.h>

#include "db.h"

const char *state_unknown = "Unknown";
const char *state_stopped = "Stopped";
const char *state_playing = "Playing";
const char *state_paused  = "Paused";





struct stat_state {
  int mpd_state;
  int song_id;
  int new_song;
};


const char *describe_playstate(int state) {
  switch (state) {
    default:
    case MPD_STATE_UNKNOWN:
      return state_unknown;
    case MPD_STATE_STOP:
      return state_stopped;
    case MPD_STATE_PLAY:
      return state_playing;
    case MPD_STATE_PAUSE:
      return state_paused;
  }
}

void stat_state_update(struct stat_state *state, struct mpd_connection *conn) {
    struct mpd_status *status = mpd_run_status(conn);
    state->mpd_state = mpd_status_get_state(status);

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
  }
}


int msleep(long msec) {
    struct timespec ts;
    int res;

    if (msec < 0) {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}


int main() {
  struct mpd_connection *mpd = mpd_connection_new(NULL, 0, 0);

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

  struct stat_state *state = malloc(sizeof(struct stat_state));
  while (1) {
    stat_state_update(state, mpd);
    run_sm(state, mpd, db);
    msleep(500);
  }
  
  mpd_connection_free(mpd);
  db_free(db);
  free(state);
  return 0;
}
