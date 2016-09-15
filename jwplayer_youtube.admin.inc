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

    // Test if we get the google API Library client.
    if (!$ytapi->checkComposerGoogleApiClientFileExists()) {
        $form['lib_error']['#markup'] = t('Google Libraries not found.<br />Use Composer Manager to download them.', array(
          '!status' => l(t('Status report'), 'admin/reports/status'),
        ));
        return $form;
    }

    $token = variable_get('jwplayer_youtube_token', array());

    // Display the authorization link.
    if (empty($token['refresh_token']) && variable_get('jwplayer_youtube_app_name', '') != '') {
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
    $form['google_app']['jwplayer_youtube_app_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Application name'),
      '#default_value' => variable_get('jwplayer_youtube_app_name', ''),
      '#required' => TRUE,
    );
    $form['google_app']['jwplayer_youtube_client_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Client ID'),
      '#default_value' => variable_get('jwplayer_youtube_client_id', ''),
      '#required' => TRUE,
    );
    $protocol = (isset($_SERVER['HTTPS'])) ? 'https' : 'http';
    $form['google_app']['jwplayer_youtube_client_secret'] = array(
      '#type' => 'textfield',
      '#title' => t('Client secret'),
      '#description' => '<em>xxx..apps.googleusercontent.com</em><br />' . t('Get credentials keys on !here <br />Next in "APIs & auth" -> "Credentials" -> "OAuth". Click "Create new Client ID" and then under "Application Type" select "Web Application".<br />For the redirect uri use !protocol :// !server_name /jwplayer_youtube/oauth2callback', array(
        '!here' => l(t('https://console.developers.google.com'), 'https://console.developers.google.com/project'),
        '!protocol' => $protocol,
        '!server_name' => $_SERVER['SERVER_NAME'])),
      '#default_value' => variable_get('jwplayer_youtube_client_secret', ''),
      '#required' => TRUE,
    );
    $country = _jwplayer_youtube_get_country();
    $countries = country_get_list();
    $form['google_app']['jwplayer_youtube_app_country'] = array(
      '#type' => 'select',
      '#title' => t('Country'),
      '#description' => t('The country is required to get video category list. If you change this setting, make sure the default category is still available after save (in "Youtube Settings" section here below)'),
      '#options' => $countries,
      '#default_value' => $country,
      '#required' => TRUE,
    );

    $form['jwplayer_youtube_autotitle'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use node title as video title'),
      '#description' => t('Synchronize the title from the node'),
      '#default_value' => variable_get('jwplayer_youtube_autotitle', FALSE),
    );

    if (variable_get('jwplayer_youtube_auth', FALSE)) {
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

    $vals = $form_state['values'];

    // If account has changed, the autorisation should be done again.
    if (variable_get('jwplayer_youtube_app_name', '') != $vals['jwplayer_youtube_app_name'] ||
        variable_get('jwplayer_youtube_client_secret', '') != $vals['jwplayer_youtube_client_secret'] ||
        variable_get('jwplayer_youtube_client_id', '') != $vals['jwplayer_youtube_client_id']
    ) {
        variable_del('jwplayer_youtube_token');
    }

    form_state_values_clean($form_state);

    // Populate variables.
    foreach ($form_state['values'] as $key => $val) {
        // Convert settings to variables.
        variable_set($key, $val);
    }
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
    $ret = array(
      '#type' => 'fieldset',
      '#title' => t('YouTube settings'),
      '#description' => t('These settings can be overriden in each field settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $ret['jwplayer_youtube_publishing_options'] = array(
      '#type' => 'radios',
      '#title' => t('Default privacy options'),
      '#description' => t('More info on !here', array('!here' => l(t('YouTube support page'), 'http://support.google.com/youtube/bin/answer.py?hl=en&answer=157177'))),
      '#options' => array('public' => t('Public'), 'private' => t('Private'), 'unlisted' => t('Unlisted')),
      '#default_value' => strtolower(variable_get('jwplayer_youtube_publishing_options', 'public')),
    );
    $ytapi = new jwplayerYTApi();
    $cats = $ytapi->getCategories();

    $ret['jwplayer_youtube_category'] = array(
      '#type' => 'select',
      '#title' => t('Default main category'),
      '#options' => $cats,
      '#description' => t('The category is required by YouTube.'),
      '#default_value' => variable_get('jwplayer_youtube_category', 0),
    );
    $ret['jwplayer_youtube_description'] = array(
      '#type' => 'textarea',
      '#title' => t('Default description'),
      '#description' => t('The description is required by YouTube.'),
      '#default_value' => variable_get('jwplayer_youtube_description', 'default description'),
      '#required' => TRUE,
    );
    $ret['jwplayer_youtube_tags'] = array(
      '#type' => 'textfield',
      '#title' => t('Default tags'),
      '#description' => t('Separate the tags by a coma. Example <em> funny, fun</em>'),
      '#default_value' => variable_get('jwplayer_youtube_tags', 'inforum.com, news'),
      '#required' => TRUE,
    );

    return $ret;
}
