<?php

/**
 *
 */
class jwplayerYTApi {

    protected $yt;
    protected $client;

    /**
     * Set library path.
     */
    public function __construct() {
        
    }

    /**
     * Determine whether the Composer file for Google API Client exists.
     */
    public function checkComposerGoogleApiClientFileExists() {
        $vendor = composer_manager_vendor_dir();
        if (file_exists($vendor . "/google/apiclient")) {
            return TRUE;
        }
        return FALSE;
    }

    /**
     * Get a authentication url.
     */
    public function getAuthUrl() {
        $this->authenticate();
        $state = mt_rand();
        $this->client->setState($state);
        $auth_url = $this->client->createAuthUrl();
        return $auth_url;
    }

    /**
     * Get a token based on returned code.
     */
    public function getTokenFromCode($code) {
        try {
            $this->authenticate();
            $this->client->authenticate($code);
            variable_set('jwplayer_youtube_token', drupal_json_decode($this->client->getAccessToken()));
        }
        catch (Google_ServiceException $e) {
            drupal_set_message(t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
        catch (Google_Exception $e) {
            drupal_set_message(t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
    }

    /**
     * Authenticate to google with account settings.
     */
    public function authenticate($credentials = array()) {

        $error = '';
        try {
            if (!isset($this->client)) {
                $app_name = !empty($credentials['app_name']) ? $credentials['app_name'] : variable_get('jwplayer_youtube_app_name', '');
                $client_secret = !empty($credentials['client_secret']) ? $credentials['client_secret'] : variable_get('jwplayer_youtube_client_secret', '');
                $client_id = !empty($credentials['client_id']) ? $credentials['client_id'] : variable_get('jwplayer_youtube_client_id', '');

                // Authenticate to Google API.
                $client = new Google_Client();
                $client->setClientId($client_id);
                $client->setClientSecret($client_secret);
                $client->setScopes('https://www.googleapis.com/auth/youtube');
                $client->setRedirectUri($GLOBALS['base_url'] . '/jwplayer_youtube/oauth2callback');
                $client->setApplicationName($app_name);
                $client->setAccessType('offline');
                $client->setApprovalPrompt('force');

                $this->yt = new Google_Service_YouTube($client);
                $this->client = $client;
            }
        }
        catch (Google_ServiceException $e) {
            $error = t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage())));
        }
        catch (Google_Exception $e) {
            $error = t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage())));
        }
        return array('error' => $error);
    }

    /**
     * Get a fresh, valid token.
     */
    public function getFreshToken() {

        $token = variable_get('jwplayer_youtube_token', array());

        if (empty($token['refresh_token'])) {
            return array('error' => t('no refresh token'));
        }

        $error = '';
        try {
            $this->authenticate();
            $this->client->setAccessToken(drupal_json_encode($token));

            // Check to see if our access token has expired.
            // If so, get a new one and save it to file for future use.
            if ($this->client->isAccessTokenExpired()) {
                $new_token = json_decode($this->client->getAccessToken());
                $this->client->refreshToken($new_token->refresh_token);
                $token = drupal_json_decode($this->client->getAccessToken());
                variable_set('jwplayer_youtube_token', $token);
            }
        }
        catch (Google_ServiceException $e) {
            $error = t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage())));
        }
        catch (Google_Exception $e) {
            $error = t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage())));
        }
        return array('token' => $token['access_token'], 'error' => $error);
    }

    /**
     * Get default thumb and actual title for a video.
     */
    public function getTitleThumbs($video_id) {

        $this->getFreshToken();
        $list_response = $this->yt->videos->listVideos("snippet", array('id' => $video_id));

        if (empty($list_response)) {
            return array('error' => t('Video %vid not found', array('%vid' => $video_id)));
        }
        $video = $list_response[0]['snippet'];
        return array('title' => $video['title'], 'default_thumb' => $video['thumbnails']['high']['url']);
    }

    /**
     * Get categories.
     */
    public function getCategories() {
        $cat = array();
        $this->getFreshToken();
        if (!empty($this->yt)) {
            $region = _jwplayer_youtube_get_country();
            $list_response = $this->yt->videoCategories->listVideoCategories('snippet', array('regionCode' => $region));
            foreach ($list_response as $resp) {
                $cat[$resp['id']] = $resp['snippet']['title'];
            }
        }
        return $cat;
    }

    /**
     * Delete a video from YouTube.
     */
    public function deleteVideo($video_id) {
        $this->getFreshToken();
        $error = '';
        try {
            $result = $this->yt->videos->delete($video_id);
        }
        catch (Google_ServiceException $e) {
            drupal_set_message(t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
        catch (Google_Exception $e) {
            drupal_set_message(t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
    }

    /**
     * Upload a video to YouTube.
     */
    public function uploadVideo($media) {
        try {
            $this->getFreshToken();

            if (empty($media)) {
                return array('error' => t('Video %vid not found', array('%vid' => $media)));
            }
            // REPLACE this value with the path to the file you are uploading.
            $videoPath = "https://content.jwplatform.com/videos/{$media->media_key}.mp4";
            $size = $this->curlFileSize($media->media_key);

            $snippet = new Google_Service_YouTube_VideoSnippet();
            $snippet->setTitle($media->title);
            $snippet->setDescription(!empty($media->description) ? $media->description : variable_get('jwplayer_youtube_description', ''));
            $snippet->setCategoryId(variable_get('jwplayer_youtube_category', 25));
            $snippet->setTags(!empty($media->tags) ? array($media->tags) : variable_get('jwplayer_youtube_tags', ''));

            // Change the privacy setting.
            $status = new Google_Service_YouTube_VideoStatus();
            $status->privacyStatus = variable_get('jwplayer_youtube_publishing_options', 'unlisted');

            // Associate the snippet and status objects with a new video resource.
            $video = new Google_Service_YouTube_Video();
            $video->setSnippet($snippet);
            $video->setStatus($status);

            // Specify the size of each chunk of data, in bytes. Set a higher value for
            // reliable connection as fewer chunks lead to faster uploads. Set a lower
            // value for better recovery on less reliable connections.
            $chunkSizeBytes = 1 * 1024 * 1024;

            // Setting the defer flag to true tells the client to return a request which can be called
            // with ->execute(); instead of making the API call immediately.
            $this->client->setDefer(true);

            // Insert the video.
            $insert_response = $this->yt->videos->insert('status,snippet', $video);

            // Create a MediaFileUpload object for resumable uploads.
            $file = new Google_Http_MediaFileUpload(
                $this->client, $insert_response, 'video/*', null, true, $chunkSizeBytes
            );
            $file->setFileSize($size);

            // Read the media file and upload it chunk by chunk.
            $status = false;

            $handle = fopen($videoPath, "rb");
            while (!$status && !feof($handle)) {
                $chunk = fread($handle, $chunkSizeBytes);
                $status = $file->nextChunk($chunk);
            }

            // The final value of $status will be the data from the API for the object that has been uploaded.
            $result = false;
            if ($status != false) {
                $result = $status;
            }

            fclose($handle);

            // If you want to make other calls after the file upload, set setDefer back to false
            $this->client->setDefer(false);
            $youtube_new_url = "https://www.youtube.com/watch?v=" . $status['id'];

            return array(true, $youtube_new_url);
        }
        catch (Google_ServiceException $e) {
            drupal_set_message(t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
        catch (Google_Exception $e) {
            drupal_set_message(t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
    }

    /**
     * Update a video on YouTube.
     */
    public function updateVideo($video_id, $field_values) {

        try {
            $this->getFreshToken();
            $list_response = $this->yt->videos->listVideos('status,snippet', array('id' => $video_id));
        }
        catch (Google_ServiceException $e) {
            drupal_set_message(t('A service error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
        catch (Google_Exception $e) {
            drupal_set_message(t('A client error occurred: <code>@error</code></p>', array('@error' => htmlspecialchars($e->getMessage()))), 'error');
        }
        if (empty($list_response)) {
            return array('error' => t('Video %vid not found', array('%vid' => $video_id)));
        }
        $video = $list_response[0];
        // Change the title.
        $video_snippet = $video['snippet'];
        $video_snippet->setTitle($field_values['title']);
        $video_snippet->setDescription($field_values['description']);
        $video_snippet->setCategoryId($field_values['category']);
        $video_snippet->setTags($field_values['tags']);
        $video->setSnippet($video_snippet);

        // Change the privacy setting.
        $video_status = $video['status'];
        $video_status->privacyStatus = $field_values['privacy'];
        $video->setStatus($video_status);

        // Update the video.
        $update_response = $this->yt->videos->update('status,snippet', $video);
    }

    public function curlFileSize($video_id) {
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, "https://content.jwplatform.com/videos/{$video_id}.mp4");
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_HEADER, true);
        curl_setopt($ch, CURLOPT_NOBODY, true);
        curl_exec($ch);
        $size = curl_getinfo($ch, CURLINFO_CONTENT_LENGTH_DOWNLOAD);
        return $size;
    }

}
