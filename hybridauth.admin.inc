<?php

function hybridauth_admin_settings() {
  $form = array();
  
  module_load_include('inc', 'hybridauth', 'hybridauth.auth');
  
  $form['hybridauth_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug Mode'),
    '#description' => t('When in debug mode, extra error information will be logged/displayed. This should be disabled when not in development.'),
    '#default_value' => variable_get('hybridauth_debug', FALSE),
  );
  
  $form['hybridauth_import_profile_photo'] = array(
    '#type' => 'checkbox',
    '#title' => t('Import Profile Photo'),
    '#description' => t('When a user first registers using a social login, should we import a profile photo, if available.'),
    '#default_value' => variable_get('hybridauth_import_profile_photo', FALSE),
  );
  
  $form['providers'] = array(
    '#type' => 'fieldset',
    '#title' => t('Providers'),
    //'#tree' => TRUE,
  );
  foreach (hybridauth_get_providers() as $provider_id => $provider_name) {
    $form['providers']['hybridauth_provider_' . $provider_id . '_enabled'] = array(
      '#type' => 'checkbox',
      '#title' => $provider_name,
      '#default_value' => variable_get('hybridauth_provider_' . $provider_id . '_enabled', FALSE),
    );
    
    $form['providers'][$provider_id] = array();
    if (in_array($provider_id, array('Facebook', 'Live'))) {
      $form['providers'][$provider_id]['hybridauth_provider_' . $provider_id . '_keys_id'] = array(
        '#type' => 'textfield',
        '#title' => t('Application ID'),
        '#description' => t('The application ID'),
        '#default_value' => variable_get('hybridauth_provider_' . $provider_id . '_keys_id', ''),
        '#states' => array(
          'invisible' => array(
            'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
          ),
        ),
      );
    }
    if (in_array($provider_id, array('Google', 'LinkedIn', 'MySpace', 'Twitter', 'YahooOAuth'))) {
      $form['providers'][$provider_id]['hybridauth_provider_' . $provider_id . '_keys_key'] = array(
        '#type' => 'textfield',
        '#title' => t('Application consumer key'),
        '#description' => t('The Application consumer key'),
        '#default_value' => variable_get('hybridauth_provider_' . $provider_id . '_keys_key', ''),
        '#states' => array(
          'invisible' => array(
            'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
          ),
        ),
      );
    }
    if (in_array($provider_id, array('Facebook', 'Google', 'LinkedIn', 'Live', 'MySpace', 'Twitter', 'YahooOAuth'))) {
      $form['providers'][$provider_id]['hybridauth_provider_' . $provider_id . '_keys_secret'] = array(
        '#type' => 'textfield',
        '#title' => t('Application consumer secret'),
        '#description' => t('The application consumer secret key'),
        '#default_value' => variable_get('hybridauth_provider_' . $provider_id . '_keys_secret', ''),
        '#states' => array(
          'invisible' => array(
            'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
          ),
        ),
      );
    }
    
    if (in_array($provider_id, array('Facebook'))) {
      $form['providers'][$provider_id]['advanced'] = array(
        '#type' => 'fieldset',
        '#title' => 'Advanced settings',
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#states' => array(
          'invisible' => array(
            'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
          ),
        ),
      );
      $form['providers'][$provider_id]['advanced']['hybridauth_provider_' . $provider_id . '_scope'] = array(
        '#type' => 'checkboxes',
        '#title' => t('Scope - Permissions to request by default'),
        '#default_value' => variable_get('hybridauth_provider_' . $provider_id . '_scope', array(
          'email', 'user_about_me', 'user_birthday', 'user_hometown', 'user_website', 'offline_access', 'read_stream', 'publish_stream', 'read_friendlists',
        )),
        '#options' => array(
          // See http://developers.facebook.com/docs/reference/api/permissions/
          // Regular permissions
          'user_about_me'             => t('<b>user_about_me</b> -- Provides access to the "About Me" section of the profile in the about property'),
          'user_activities'           => t('<b>user_activities</b> -- Provides access to the user\'s list of activities as the activities connection'),
          'user_birthday'             => t('<b>user_birthday</b> -- Provides access to the birthday with year as the birthday_date property'),
          'user_checkins'             => t('<b>user_checkins</b> -- Provides read access to the authorized user\'s check-ins that the user can see.'),
          'user_education_history'    => t('<b>user_education_history</b> -- Provides access to education history as the education property'),
          'user_events'               => t('<b>user_events</b> -- Provides access to the list of events the user is attending as the events connection'),
          'user_groups'               => t('<b>user_groups</b> -- Provides access to the list of groups the user is a member of as the groups connection'),
          'user_hometown'             => t('<b>user_hometown</b> -- Provides access to the user\'s hometown in the hometown property'),
          'user_interests'            => t('<b>user_interests</b> -- Provides access to the user\'s list of interests as the interests connection'),
          'user_likes'                => t('<b>user_likes</b> -- Provides access to the list of all of the pages the user has liked as the likes connection'),
          'user_location'             => t('<b>user_location</b> -- Provides access to the user\'s current location as the location property'),
          'user_notes'                => t('<b>user_notes</b> -- Provides access to the user\'s notes as the notes connection'),
          'user_online_presence'      => t('<b>user_online_presence</b> -- Provides access to the user\'s online/offline presence'),
          'user_photos'               => t('<b>user_photos</b> -- Provides access to the photos the user has uploaded, and photos the user has been tagged in'),
          'user_questions'            => t('<b>user_questions</b> -- Provides access to the questions the user or friend has asked'),
          'user_relationships'        => t('<b>user_relationships</b> -- Provides access to the user\'s family and personal relationships and relationship status'),
          'user_relationship_details' => t('<b>user_relationship_details</b> -- Provides access to the user\'s relationship preferences'),
          'user_religion_politics'    => t('<b>user_religion_politics</b> -- Provides access to the user\'s religious and political affiliations'),
          'user_status'               => t('<b>user_status</b> -- Provides access to the user\'s most recent status message'),
          'user_videos'               => t('<b>user_videos</b> -- Provides access to the videos the user has uploaded, and videos the user has been tagged in'),
          'user_website'              => t('<b>user_website</b> -- Provides access to the user\'s web site URL'),
          'user_work_history'         => t('<b>user_work_history</b> -- Provides access to work history as the work property'),
          'email'                     => t('<b>email</b> -- Provides access to the user\'s primary email address in the email property. Do not spam users. Your use of email must comply both with Facebook policies and with the CAN-SPAM Act.'),
          
          // Extended permissions
          'read_friendlists'      => t('<b>read_friendlists</b> -- Provides access to any friend lists the user created. All user\'s friends are provided as part of basic data, this extended permission grants access to the lists of friends a user has created, and should only be requested if your application utilizes lists of friends.'),
          'read_insights'         => t('<b>read_insights</b> -- Provides read access to the Insights data for pages, applications, and domains the user owns.'),
          'read_mailbox'          => t('<b>read_mailbox</b> -- Provides the ability to read from a user\'s Facebook Inbox.'),
          'read_requests'         => t('<b>read_requests</b> -- Provides read access to the user\'s friend requests'),
          'read_stream'           => t('<b>read_stream</b> -- Provides access to all the posts in the user\'s News Feed and enables your application to perform searches against the user\'s News Feed'),
          'xmpp_login'            => t('<b>xmpp_login</b> -- Provides applications that integrate with Facebook Chat the ability to log in users.'),
          'ads_management'        => t('<b>ads_management</b> -- Provides the ability to manage ads and call the Facebook Ads API on behalf of a user.'),
          'create_event'          => t('<b>create_event</b> -- Enables your application to create and modify events on the user\'s behalf'),
          'manage_friendlists'    => t('<b>manage_friendlists</b> -- Enables your app to create and edit the user\'s friend lists.'),
          'manage_notifications'  => t('<b>manage_notifications</b> -- Enables your app to read notifications and mark them as read. This permission will be required to all access to notifications after October 22, 2011.'),
          'offline_access'        => t('<b>offline_access</b> -- Enables your app to perform authorized requests on behalf of the user at any time. By default, most access tokens expire after a short time period to ensure applications only make requests on behalf of the user when the are actively using the application. This permission makes the access token returned by our OAuth endpoint long-lived.'),
          'publish_checkins'      => t('<b>publish_checkins</b> -- Enables your app to perform checkins on behalf of the user.'),
          'publish_stream'        => t('<b>publish_stream</b> -- Enables your app to post content, comments, and likes to a user\'s stream and to the streams of the user\'s friends. With this permission, you can publish content to a user\'s feed at any time, without requiring offline_access. However, please note that Facebook recommends a user-initiated sharing model.'),
          'rsvp_event'            => t('<b>rsvp_event</b> -- Enables your application to RSVP to events on the user\'s behalf'),
          'sms'                   => t('<b>sms</b> -- Enables your application to send messages to the user and respond to messages from the user via text message'),
          'publish_actions'       => t('<b>publish_actions</b> -- Enables your application to publish user scores and achievements.'),
        ),
        '#states' => array(
          'invisible' => array(
            'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
          ),
        ),
      );
    }
    
    if (!empty($form['providers'][$provider_id])) {
      $form['providers'][$provider_id]['#type'] = 'fieldset';
      $form['providers'][$provider_id]['#title'] = $provider_name . ' settings';
      $form['providers'][$provider_id]['#collapsible'] = TRUE;
      $form['providers'][$provider_id]['#collapsed'] = FALSE;
      $form['providers'][$provider_id]['#states'] = array(
        'invisible' => array(
          'input[name="hybridauth_provider_' . $provider_id . '_enabled"]' => array('checked' => FALSE),
        ),
      );
    }
  }
  
  return system_settings_form($form);
}
