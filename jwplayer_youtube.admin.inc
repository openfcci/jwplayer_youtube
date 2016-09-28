<?php

/**
 * @file
 * Administrative page callbacks for the youtube_upload module.
 */

/**
 * General configuration form for youtube (account info + youtube settings).
 */
function jwplayer_youtube_account_settings_form($form, $form_state) {
    include_once DRUPAL_ROOT . '/includes/locale.inc';

    //Create new object
    $ytapi = new jwplayerYTApi();

    //Credentials
    $credentials = json_decode(variable_get('youtubeapi', ''));
    if (!$credentials->client_id && !$credentials->client_secret) {
        $credentials = _get_default_credentials();
    }

    // Test if we get the google API Library client.
    if (!$ytapi->checkComposerGoogleApiClientFileExists()) {
        $form['lib_error']['#markup'] = t('Google Libraries not found.<br />Use Composer Manager to download them.', array(
          '!status' => l(t('Status report'), 'admin/reports/status'),
        ));
        return $form;
    }

    $token = variable_get('googleapi_token', array());

    // Display the authorization link.
    if (empty($token['refresh_token']) && $credentials->app_name) {
        // Test the connection to google.
        $res = $ytapi->getAuthUrl();
        drupal_set_message(t('You need to allow your application by following !link_auth', array('!link_auth' => l(t('this link'), $res))), 'warning', FALSE);
    }
    elseif (!empty($token['refresh_token'])) {
        drupal_set_message('Token acquired from Google', 'status', FALSE);
    }

    $form['google_app'] = array(
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#title' => t('Google Application'),
    );
    $form['google_app']['app_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Application name'),
      '#default_value' => $credentials->app_name,
      '#required' => TRUE,
    );
    $form['google_app']['client_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Client ID'),
      '#default_value' => $credentials->client_id,
      '#required' => TRUE,
    );
    $protocol = (isset($_SERVER['HTTPS'])) ? 'https' : 'http';
    $form['google_app']['client_secret'] = array(
      '#type' => 'textfield',
      '#title' => t('Client secret'),
      '#description' => '<em>xxx..apps.googleusercontent.com</em><br />' . t('Get credentials keys on !here <br />Next in "APIs & auth" -> "Credentials" -> "OAuth". Click "Create new Client ID" and then under "Application Type" select "Web Application".<br />For the redirect uri use !protocol :// !server_name /jwplayer_youtube/oauth2callback', array(
        '!here' => l(t('https://console.developers.google.com'), 'https://console.developers.google.com/project'),
        '!protocol' => $protocol,
        '!server_name' => $_SERVER['SERVER_NAME'])),
      '#default_value' => $credentials->client_secret,
      '#required' => TRUE,
    );

    $countries = country_get_list();
    $form['google_app']['app_country'] = array(
      '#type' => 'select',
      '#title' => t('Country'),
      '#description' => t('The country is required to get video category list. If you change this setting, make sure the default category is still available after save (in "Youtube Settings" section here below)'),
      '#options' => $countries,
      '#default_value' => $credentials->app_country,
      '#required' => TRUE,
    );

    if (variable_get('googleapi_auth', FALSE)) {
        $form['youtube_settings'] = jwplayer_youtube_get_settings_form();
    }

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('save configuration'),
    );

    return $form;
}

/**
 * Implements hook_form_submit().
 */
function jwplayer_youtube_account_settings_form_submit($form, &$form_state) {

    $credentials = json_decode(variable_get('youtubeapi', ''));

    form_state_values_clean($form_state);
    // Populate variables.
    foreach ($form_state['values'] as $key => $val) {
        // If account has changed, the autorization should be done again.
        if ($credentials->{$key} !== $form_state['values'][$key]) {
            variable_del('googleapi_token');
        }
        $credentials->{$key} = $val;
    }
    variable_set('youtubeapi', json_encode($credentials));
}

/**
 * Returns the default credentials object.
 */
function _get_default_credentials() {

    $default = (object) array(
          'app_name' => '',
          'client_id' => '',
          'client_secret' => '',
          'app_country' => _jwplayer_youtube_get_country(),
          'publishing_options' => 'public',
          'category' => 25,
          'description' => 'default description',
          'tags' => 'inforum.com, news'
    );

    return $default;
}

/**
 * Helper.
 *
 * get form field for the youtube settings
 * (used on at the module and the field level)
 *
 * @see jwplayer_youtube_menu()
 */
function jwplayer_youtube_get_settings_form() {

    //Credentials
    $default = _get_default_credentials();

    $ret['youtube_upload'] = array(
      '#type' => 'fieldset',
      '#title' => t('YouTube settings'),
      '#description' => t('These settings can be overriden in each field settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $ret['youtube_upload']['default_publishing_options'] = array(
      '#type' => 'radios',
      '#title' => t('Default privacy options'),
      '#description' => t('More info on !here', array('!here' => l(t('YouTube support page'), 'http://support.google.com/youtube/bin/answer.py?hl=en&answer=157177'))),
      '#options' => array('public' => t('Public'), 'private' => t('Private'), 'unlisted' => t('Unlisted')),
      '#default_value' => $default->publishing_options,
    );
    $ytapi = new jwplayerYTApi();
    $cats = $ytapi->getCategories();

    $ret['youtube_upload']['default_category'] = array(
      '#type' => 'select',
      '#title' => t('Default main category'),
      '#options' => $cats,
      '#description' => t('The category is required by YouTube.'),
      '#default_value' => $default->category,
    );
    $ret['youtube_upload']['default_description'] = array(
      '#type' => 'textarea',
      '#title' => t('Default description'),
      '#description' => t('The description is required by YouTube.'),
      '#default_value' => $default->description,
      '#required' => TRUE,
    );
    $ret['youtube_upload']['default_tags'] = array(
      '#type' => 'textfield',
      '#title' => t('Default tags'),
      '#description' => t('Separate the tags by a coma. Example <em> funny, fun</em>'),
      '#default_value' => $default->tags,
      '#required' => TRUE,
    );

    return $ret;
}
