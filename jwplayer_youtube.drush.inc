<?php

/**
 * @file jwplayer_youtube.drush.inc
 * Drush Commands
 */

# Load Includes
//include_once 'progress.drush.inc';

/**
 * Implements hook_drush_command().
 */
function jwplayer_youtube_drush_command() {
    $items = array();
    $items['migrate-jw'] = array(
      'description' => 'Migrate one or all of the jwplayer videos to youtube.',
      'examples' => array(
        'drush migrate-jw' => dt('Migrate all jwplayer videos to youtube'),
        'drush jwm' => dt('Alias to migrate all jwplayer videos to youtube.'),
        'drush es-i 100' => dt("Migrate !limit videos.", array('!limit' => 100)),
        'drush es-i 100 10' => dt("Migrate !limit videos by offset !offset.", array('!limit' => 100, '!offset' => 10)),
        'drush es-i 0 100' => dt("Migrate all videos with offset !offset.", array('!offset' => 10)),
      ),
      'arguments' => array(
        'limit' => dt("The number of items to index (index's cron batch size items per run). Set to 0 to index all items. Defaults to 0 (index all)."),
        'offset' => dt("The number to items to skip. Defaults to 0 (index from beginning)."),
      ),
      'aliases' => array('jwm'),
    );

    return $items;
}

/**
 * 
 * @param type $limit
 * @param type $offset
 */
function drush_jwplayer_youtube_migrate_jw($limit = NULL, $offset = 0, $batch_size = 1) {
        // Get total count if limit isn't set
        if (empty($limit)) {
            $limit = jwplayer_get_video_total();
        }
        _drush_jw_migrate_batch_upload($limit, $offset, $batch_size);
}

/**
 * 
 * @param type $limit
 * @param type $offset
 */
function _drush_jw_migrate_batch_upload($limit, $offset, $batch_size) {
    // Get the number of remaining items to index.
    try {
        //Create index and mapping
        for ($count = 0; $count <= $limit; $count++) {
            if ($count % $batch_size == 0) { // Every n documents stop and send the bulk request
                // Build elastic search query
                $document = jwplayer_youtube_get_video_data_bulk($offset, $batch_size);
                $responses = _jwplayer_youtube_upload_videos_bulk($document);

                $offset += $batch_size;
                unset($document);  // erase the old bulk request
                unset($responses); // unset  to save memory
            }
        }
    }
    catch (Exception $ex) {
        drush_log($ex->getMessage(), 'error');
    }
}
