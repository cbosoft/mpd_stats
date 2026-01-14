#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "playlist.h"
#include "msleep.h"

struct uri_node;
struct uri_node {
  char *uri;
  struct uri_node *next;
};

struct uri_node *uri_node_create(const char *uri) {
  struct uri_node *node = malloc(sizeof(struct uri_node));
  int n = strlen(uri);
  node->uri = malloc((n+1)*sizeof(char));
  strncpy(node->uri, uri, n);
  node->uri[n] = 0;
  node->next = NULL;
  return node;
}

struct uri_node *uri_node_append(struct uri_node *node, const char *uri) {
  node->next = uri_node_create(uri);
  return node->next;
}

void uri_node_free(struct uri_node *root) {
  struct uri_node *ptr = root;
  while (ptr != NULL) {
    struct uri_node *next = ptr-> next;
    free(ptr->uri);
    free(ptr);

    ptr = next;
  }
}

void generate_playlist(struct mpd_connection *mpd, struct db_conn *db, int (*f)(struct db_conn *, char ***), int tag, const char *playlist_name, bool top) {
  char **results = NULL;
  int n_results = f(db, &results);
  fprintf(stderr, "%s :: %d\n", playlist_name, n_results);

  int n_effective_results = n_results;
  if (top) n_effective_results = 1;

  if (!mpd_run_playlist_clear(mpd, playlist_name)) mpd_connection_clear_error(mpd);

  struct uri_node *root = NULL, *ptr = NULL;

  for (int i = 0; i < n_effective_results; i++) {
    fprintf(stderr, "%d, %s\n", i, results[i]);
    // if (!mpd_search_db_songs(mpd, 1)) goto _generate_playlist_error;
    if (!mpd_search_add_db_songs_to_playlist(mpd, playlist_name)) goto _generate_playlist_error;
    if (!mpd_search_add_tag_constraint(mpd, MPD_OPERATOR_DEFAULT, tag, results[i])) goto _generate_playlist_error;
    if (!mpd_search_commit(mpd)) goto _generate_playlist_error;

    struct mpd_pair *pair = mpd_recv_pair(mpd);
    struct mpd_song *song = NULL;
    while (pair != NULL) {
      if (song == NULL) {
        song = mpd_song_begin(pair);
      }
      mpd_return_pair(mpd, pair);
      // free(pair);
      pair = mpd_recv_pair(mpd);

      if (pair == NULL || !mpd_song_feed(song, pair)) {
        const char *uri = mpd_song_get_uri(song);
        if (ptr == NULL) {
          root = uri_node_create(uri);
          ptr = root;
        }
        else {
          ptr = uri_node_append(ptr, uri);
        }
        mpd_song_free(song);
        song = NULL;
      }
    }
  }
  

  for (ptr = root; ptr != NULL; ptr = ptr->next) {
    //fprintf(stderr, "got song uri for %s: %s\n", playlist_name, ptr->uri);

    // small delay required to make things work
    msleep(10);
    // praise the small delay

    if (!mpd_run_playlist_add(mpd, playlist_name, ptr->uri)) goto _generate_playlist_error;
  }

  uri_node_free(root);
  db_free_results(results, n_results);
  return;

_generate_playlist_error:
  fprintf(stderr, "error encountered in generate_playlist \"%s\": %s\n", playlist_name, mpd_connection_get_error_message(mpd));
  mpd_connection_clear_error(mpd);
  uri_node_free(root);
  db_free_results(results, n_results);
}

void generate_playlists(struct mpd_connection *mpd, struct db_conn *db) {
  // generate_playlist(mpd, db, db_fetch_frequent_songs, MPD_TAG_TITLE, "Most Played Songs", 0);
  generate_playlist(mpd, db, db_fetch_frequent_albums, MPD_TAG_ALBUM, "Most Played Albums", 0);
  //generate_playlist(mpd, db, db_fetch_frequent_artists, MPD_TAG_ARTIST, "Most Played Artists", 0);

  //generate_playlist(mpd, db, db_fetch_recent_songs, MPD_TAG_TITLE, "Recently Played Songs", 0);
  generate_playlist(mpd, db, db_fetch_recent_albums, MPD_TAG_ALBUM, "Recently Played Albums", 0);
  generate_playlist(mpd, db, db_fetch_recent_artists, MPD_TAG_ARTIST, "From Recently Played Artists", 0);
  generate_playlist(mpd, db, db_fetch_recent_albums, MPD_TAG_ALBUM, "Last Played Album", 1);
  generate_playlist(mpd, db, db_fetch_recent_artists, MPD_TAG_ARTIST, "From Last Played Artists", 1);
}
