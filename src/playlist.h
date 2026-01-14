#pragma once

#include "db.h"
#include <mpd/client.h>

void generate_playlists(struct mpd_connection *mpd, struct db_conn *db);
