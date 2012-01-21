<?php

function hybridauth_endpoint() {
  // Make sure it's Drupal that starts the session.
  drupal_session_start();
  
  $lib_path = libraries_get_path('hybridauth', FALSE);
  if ($lib_path) {
    try {
      require_once($lib_path . '/index.php');
    }
    catch(Exception $e) {
      // TODO: How should we handle this?
      watchdog_exception('hybridauth', $e);
    }
  }
  
  return MENU_ACCESS_DENIED;
}

function hybridauth_popup() {
  global $user;
  
  $provider_id = isset($_REQUEST['provider']) ? $_REQUEST['provider'] : NULL;
  
  // If no provider was given then look for saved preference
  if (empty($provider_id) || $provider_id == 'none') {
    if (isset($_COOKIE['__ha_provider']) && !empty($_COOKIE['__ha_provider'])) {
      $provider_id = $_COOKIE['__ha_provider'];
    }
  }
  // Else, save provider preference to a cookie for 365 days.
  else {
    setcookie('__ha_provider', $provider_id, REQUEST_TIME + 31536000, base_path(), variable_get('cookie_domain', ''));
  }
  
  // Make sure it's Drupal that starts the session then load the lib
  drupal_session_start();
  module_load_include('inc', 'hybridauth', 'hybridauth.auth');
  
  // Try to get HybridAuth instance
  try {
    $hybridauth = hybridauth_get_instance();
  }
  // PHP error
  catch (ErrorException $e) {
    drupal_set_message(t('There was an error processing your request!'), 'error');
    watchdog_exception('hybridauth', $e);
    
    // Cleanly close popup and redirect
    $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
    $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
    drupal_add_js('
      if (window.opener){
        try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
        window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
      }
      window.self.close();
    ', 'inline');
    $page = array(
      'page_top' => '',
      '#children' => 'Closing...',
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  // An error coming from HA
  catch (Exception $e) {
    //watchdog_exception('hybridauth', $e);
    $redirect = TRUE;
    
    switch ($e->getCode()) {
      case 5: // Authentification failed. The user has canceled the authentication or the provider refused the connection.
        // Close popup window and leave overlay alone.
        $redirect = FALSE;
        break;
        
      case 0: // Unspecified error.
      case 1: // Hybriauth configuration error.
      case 2: // Provider not properly configured.
      case 3: // Unknown or disabled provider.
      case 4: // Missing provider application credentials.
      default:
        // Report the error and log it.
        drupal_set_message(t('There was an error processing your request!'), 'error');
        watchdog_exception('hybridauth', $e);
    }
    
    // Cleanly close popup and redirect
    $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
    $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
    drupal_add_js('
      var redirect = ' . ($redirect ? 'true' : 'false') . ';
      if (window.opener && redirect) {
        try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
        window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
      }
      window.self.close();
    ', 'inline');
    $page = array(
      'page_top' => '',
      '#children' => 'Closing...',
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  // If we don't have a provider ID show a list of providers.
  if (empty($provider_id) || $provider_id == 'none' || $provider_id == 'list') {
    return _hybridauth_popup_list($hybridauth);
  }
  // Else if provider is OpenID, but we don't have the OpenID target, show OpenId form.
  elseif ($provider_id == 'OpenID' && !isset($_GET['openid_identifier'])) {
    $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
    unset($_GET['destination']);
    
    $query = drupal_get_query_parameters();
    $query['destination'] = $destination;
    
    drupal_goto('hybridauth/popup/openid', array('query' => $query));
  }
  // Should we try to run the authentication?
  elseif (isset($_GET['authenticate']) && $_GET['authenticate']) {
    return _hybridauth_popup_auth($hybridauth, $provider_id);
  }
  // Else let's show the provider popup
  else {
    return _hybridauth_popup_provider($hybridauth, $provider_id);
  }
  
  // We shouldn't get here
  return MENU_ACCESS_DENIED;
}

function _hybridauth_popup_list($hybridauth) {
  _hybridauth_add_widget_css();
  $build = array(
    '#markup' => theme('hybridauth_popup_list', array()),
  );
  
  // Are we working inside a colorbox overlay?
  if (isset($_GET['overlay']) && $_GET['overlay']) {
    $page = array(
      'page_top' => '',
      '#children' => drupal_render($build),
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  return $build;
}

function _hybridauth_popup_provider($hybridauth, $provider_id) {
  _hybridauth_add_widget_css();
  $build = array(
    '#markup' => theme('hybridauth_popup_provider', array('provider_id' => $provider_id, 'done' => TRUE)),
  );
  
  // Are we working inside a colorbox overlay?
  if (isset($_GET['overlay']) && $_GET['overlay']) {
    $page = array(
      'page_top' => '',
      '#children' => drupal_render($build),
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  return $build;
}

function _hybridauth_popup_auth($hybridauth, $provider_id) {
  $_SESSION['hybridauth'] = array();
  $_SESSION['hybridauth']['stat'] = 'ok';
  
  $params = array();
  if (isset($_GET['openid_identifier'])) {
    $params['openid_identifier'] = $_GET['openid_identifier'];
  }
  
  try {
    $adapter = $hybridauth->authenticate($provider_id, $params);
    $profile = (array) ($adapter->getUserProfile());
  }
  catch(Exception $e) {
    $_SESSION['hybridauth']['stat'] = $e->getMessage();
    drupal_set_message(t('We were unable to complete your request.'), 'error');
    watchdog_exception('hybridauth', $e);
    
    // Cleanly close popup and redirect
    $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
    $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
    drupal_add_js('
      if (window.opener){
        try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
        window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
      }
      window.self.close();
    ', 'inline');
    $page = array(
      'page_top' => '',
      '#children' => 'Closing...',
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  // If we got here then success!
  return _hybridauth_popup_process_auth($hybridauth, $adapter, $profile, $provider_id);
}

/**
 * A helper function that takes a successful HA authentication and handles the Drupal side of things.
 */
function _hybridauth_popup_process_auth($hybridauth, $adapter, $profile, $provider_id) {
  global $user;
  
  $provider_name = hybridauth_get_provider_name($provider_id);
  $_SESSION['hybridauth']['profile'] = $profile;
  
  // Save provider info (for token replacement and account linking).
  $_SESSION['hybridauth_provider_info'] = array(
    'id' => $provider_id,
    'name' => $provider_name,
  );
  
  // Are we adding a new identity to an existing account?
  if (isset($_GET['add']) && $_GET['add']) {
    if (user_is_logged_in()) {
      return _hybridauth_popup_process_auth_addexisting($hybridauth, $adapter, $profile, $provider_id);
    }
  }
  
  // This wasn't a request to add to an existing logged-in account
  // Let's see if it matches an existing account that is not logged in.
  $account = user_external_load(_hybridauth_encode_authname($provider_id, $profile['identifier']));
  
  // Is this a registered user?
  if (isset($account->uid)) {
    if (!variable_get('user_email_verification', TRUE) ||
        $account->login ||
        !empty($account->data['hybridauth_data']['profile']['emailVerified']) &&
        strtolower($account->data['hybridauth_data']['profile']['emailVerified']) == strtolower($account->mail)) {
      // IF settings do not require email verification
      // OR
      // it's not the first login for the user (which means the email has
      // already been verified)
      // OR
      // they are using an email the ID provider has already verified
      //
      // then we can skip the email verification process

      // Check that the user has not been blocked.
      $state['values']['name'] = $account->name;
      user_login_name_validate(array(), $state);
      if (!form_get_errors()) {
        // Load global $user and perform final login tasks.
        $form_state['uid'] = $account->uid;
        user_login_submit(array(), $form_state);
      }
    }
    else {
      drupal_set_message(t('You must validate your email address for this account before logging in with it.'), 'error');
    }
    
    // Cleanly close popup and redirect
    $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
    $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
    drupal_add_js('
      if (window.opener){
        try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
        window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
      }
      window.self.close();
    ', 'inline');
    $page = array(
      'page_top' => '',
      '#children' => 'Closing...',
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  // New user.
  else {
    // Check that users are allowed to register on their own.
    if (variable_get('user_register', 1)) {
      if (!variable_get('hybridauth_force_registration_form', FALSE)) {
        $form_state['values'] = array();
        $form_state['values']['op'] = t('Create new account');
        drupal_form_submit('user_register_form', $form_state);
        // See if the user was successfully registered.
        if (!empty($form_state['user'])) {
          // Let other modules know that a linked account has been added.
          $account = array(
            'user' => $user,
            'id' => _hybridauth_encode_authname($provider_id, $profile['identifier']),
            'provider_id' => $provider_id,
            'provider_name' => $provider_name,
          );
          module_invoke_all('hybridauth_linked_account', 'insert', $account);
          
          // Cleanly close popup and redirect
          $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
          $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
          drupal_add_js('
            if (window.opener){
              try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
              window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
            }
            window.self.close();
          ', 'inline');
          $page = array(
            'page_top' => '',
            '#children' => 'Closing...',
            'page_bottom' => '',
          );
          print theme('html', array('page' => $page));
          drupal_exit();
        }
        // get the error messages and clear the messages queue
        $messages = drupal_get_messages('error');

        if (empty($form_state['values']['mail'])) {
          // If the idenitity provider did not provide an email address, ask
          // the user to complete (and submit) the form manually instead of
          // showing the error messages about the missing values generated by
          // FAPI.
          drupal_set_message(t('Although we have verified your account, @provider did not provide us with your e-mail address.  Please enter one below to complete your registration.  (If you\'ve previously registered with us, please <a href="@login">log in</a> and add your @provider account under "Linked accounts.")', array('@provider' => $provider_name, '@login' => url('user/login'))), 'warning');
        }
        else {
          drupal_set_message(t('Although we have verified your account, registration using the information provided by @provider failed due to the reasons listed below. Please complete the registration by filling out the form below. (If you\'ve previously registered with us, please <a href="@login">log in</a> and add your @provider account under "Linked accounts.")', array('@provider' => $provider_name, '@login' => url('user/login'))), 'warning');
          
          // Append form validation errors below the above warning.
          foreach ($messages['error'] as $message) {
            drupal_set_message($message, 'error');
          }
        }
      }
      else {
        drupal_set_message(t('Please complete the registration by filling out the form below.  (If you\'ve previously registered with us, please <a href="@login">log in</a> and add your @provider account under "Linked accounts.")', array('@provider' => $provider_name, '@login' => url('user/login'))), 'warning');
      }
      
      // Redirect to the normal registration page and prefill with the values
      // we received from HybridAuth.
      $destination = drupal_get_destination();
      unset($_GET['destination']);
      
      // Cleanly close popup and redirect
      $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
      drupal_add_js('
        if (window.opener){
          try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
          window.opener.parent.location.href = "' . url('user/register', array('query' => $destination)) . '";
        }
        window.self.close();
      ', 'inline');
      $page = array(
        'page_top' => '',
        '#children' => 'Closing...',
        'page_bottom' => '',
      );
      print theme('html', array('page' => $page));
      drupal_exit();
    }
    // Users are not allowed to register.
    else {
      drupal_set_message(t('Only site administrators can create new user accounts.'), 'error');
      
      // Cleanly close popup and redirect
      $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
      $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
      drupal_add_js('
        if (window.opener){
          try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
          window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
        }
        window.self.close();
      ', 'inline');
      $page = array(
        'page_top' => '',
        '#children' => 'Closing...',
        'page_bottom' => '',
      );
      print theme('html', array('page' => $page));
      drupal_exit();
    }
  }
  
  // We shouldn't get here, but just in case...
  return MENU_ACCESS_DENIED;
}

/**
 * A helper function that takes a successful HA authentication and handles the Drupal side of things.
 */
function _hybridauth_popup_process_auth_addexisting($hybridauth, $adapter, $profile, $provider_id) {
  global $user;
  
  // The identifier given should not be already mapped to an existing account
  if (user_get_authmaps(_hybridauth_encode_authname($provider_id, $profile['identifier']))) {
    $message = array(
      'text' => t('We were unable to complete your request.  That account ID is already linked to a user on this site.'),
      'type' => 'error',
    );
  }
  else {
    $txn = db_transaction();
    try {
      // Can't use user_set_authmaps() here, since it doesn't support multiple authnames per user via same module
      $result1 = $aid = db_insert('authmap')
        ->fields(array(
          'uid' => $user->uid,
          'authname' => _hybridauth_encode_authname($provider_id, $profile['identifier']),
          'module' => 'hybridauth',
        ))
      ->execute();
      $result2 = db_insert('hybridauth_account')
        ->fields(array(
          'aid' => $aid,
          'provider_id' => $provider_id,
          'created' => REQUEST_TIME,
        ))
        ->execute();
    }
    catch (Exception $e) {
      $txn->rollback();
      watchdog_exception('hybridauth', $e);
    }

    // Default (error) message.
    $message = array(
      'text' => t('We were unable to link your %provider account.', array('%provider' => $provider_name)),
      'type' => 'error',
    );
    if (isset($result1) && isset($result2)) {
      $message = array(
        'text' => t('We have successfully linked your %provider account.', array('%provider' => $provider_name)),
        'type' => 'status',
      );
      // Let other modules know that a linked account has been added.
      $account = array(
        'user' => $user,
        'id' => _hybridauth_encode_authname($provider_id, $profile['identifier']),
        'provider_id' => $provider_id,
        'provider_name' => $provider_name,
      );
      module_invoke_all('hybridauth_account', 'insert', $account);
      
      // TODO: support dynamic profile field mapping
      //_hybridauth_import_user_data();
    }
  }
  
  // Cleanly close popup and redirect
  drupal_set_message($message['text'], $message['type']);
  
  $GLOBALS['devel_shutdown'] = FALSE; // Prevent devel module from spewing.
  $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user/' . $user->uid . '/hybridauth';
  drupal_add_js('
    if (window.opener){
      try { window.opener.parent.$.colorbox.close(); } catch(err) {} 
      window.opener.parent.location.href = "' . url($destination, array('absolute' => TRUE)) . '";
    }
    window.self.close();
  ', 'inline');
  $page = array(
    'page_top' => '',
    '#children' => 'Closing...',
    'page_bottom' => '',
  );
  print theme('html', array('page' => $page));
  drupal_exit();
}

function hybridauth_popup_openid() {
  $overlay = isset($_GET['overlay']) ? $_GET['overlay'] : FALSE;
  
  $provider_id = isset($_GET['provider']) ? $_GET['provider'] : NULL;
  if (!$provider_id) {
    // TODO: handle overlay case
    drupal_not_found();
    drupal_exit();
  }
  
  _hybridauth_add_widget_css();
  $form = drupal_get_form('hybridauth_popup_openid_form', $provider_id, $overlay);
  
  // Are we working inside a colorbox overlay?
  if ($overlay) {
    $page = array(
      'page_top' => '',
      '#children' => drupal_render($form),
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  return drupal_render($form);
}

function hybridauth_popup_openid_form($form, &$form_state, $provider_id, $overlay=FALSE) {
  $form = array();
  
  $form['openid_identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('OpenID Identity'),
    '#description' => t('Type your OpenID identity that you want to use.'),
  );
  
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  if ($overlay) {
    // TODO: how should we handle this?
  }
  else {
    $form['actions']['cancel'] = array(
      '#type' => 'link',
      '#title' => t('Cancel'),
      '#href' => $_GET['destination'],
      //'#options' => $options,
    );
  }
  
  return $form;
}

function hybridauth_popup_openid_form_submit($form, &$form_state) {
  $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
  unset($_GET['destination']);
  
  $query = drupal_get_query_parameters();
  $query['openid_identifier'] = $form_state['values']['openid_identifier'];
  $query['destination'] = $destination;
  
  drupal_goto('hybridauth/popup', array('query' => $query));
}

/**
 * Menu callback; confirm email for HybridAuth registrations that require it.
 *
 * @see hybridauth_mail()
 * @see _hybridauth_mail_text()
 */
function hybridauth_email_confirm($uid, $timestamp, $hashed_pass) {
  global $user;

  // Make sure that a user isn't already logged in.
  if ($user->uid) {
    // The user is already logged in
    if ($user->uid == $uid) {
      drupal_set_message(t('You have already used this email confirmation link and you are already logged in.'));
      drupal_goto();
    }
    // A different user is already logged in on the computer.
    else {
      $reset_link_account = user_load($uid);
      if (!empty($reset_link_account)) {
        drupal_set_message(t('Another user (%other_user) is already logged into the site on this computer, but you tried to use a one-time link for user %resetting_user. Please <a href="!logout">logout</a> and try using the link again.',
          array('%other_user' => $user->name, '%resetting_user' => $reset_link_account->name, '!logout' => url('user/logout'))));
      } else {
        // Invalid one-time link specifies an unknown user.
        drupal_set_message(t('The one-time login link you clicked is invalid.'));
      }
    }
    drupal_goto();
  }
  else {
    $user = user_load_multiple(array($uid), array('status' => 1));
    if ($account = array_shift($user)) {
      if ($account->login) {
        drupal_set_message(t('Your email address has already been confirmed and you may login at any time.'));
        drupal_goto('user');
      }
      elseif ($hashed_pass == user_pass_rehash($account->pass, $timestamp, $account->login)) {
        // Load global $user and perform final login tasks.
        $form_state['uid'] = $account->uid;
        user_login_submit(array(), $form_state);
        drupal_set_message(t('Thank you for confirming your email address.'));
        drupal_goto('user/' . $user->uid . '/edit');
      }
    }
  }
  // If all else fails, deny access.
  drupal_access_denied();
}

/**
 * Menu callback; manage Engage 3rd party identities for the specified user.
 */
function hybridauth_user_identities($account) {
  module_load_include('inc', 'hybridauth', 'hybridauth.auth');
  
  drupal_set_title(format_username($account));
  
  $header = array(t('Account type'), t('Account ID'), t('Operations'));
  $rows = array();
  
  $result = db_query("SELECT am.aid, am.authname, ha.provider_id FROM {authmap} am INNER JOIN {hybridauth_account} ha ON am.aid = ha.aid WHERE module = :module AND uid = :uid", array(':module' => 'hybridauth', ':uid' => $account->uid));
  
  foreach ($result as $identity) {
    $provider_id = $identity->provider_id;
    $provider_name = hybridauth_get_provider_name($provider_id);
    
    $rows[] = array(
      theme('hybridauth_provider_icon', array('provider_id' => $provider_id, 'style' => 'hybridauth-icon-inline')) . '<span' . drupal_attributes(array('class' => 'hybridauth-provider-name')) . '>' . $provider_name .  '</span>',
      check_plain(_hybridauth_get_authmap_identfier($identity->authname)),
      l(t('Delete'), 'user/' . $account->uid . '/hybridauth/delete/' . $identity->aid),
    );
  }
  
  // TODO: Look at RPX overlay code.
  /*
  hybridauth_js(FALSE);
  $realm = variable_get('hybridauth_realm', '');
  $realm_scheme = variable_get('hybridauth_realm_scheme', 'http');
  $sign_in_url = "$realm_scheme://$realm/openid/v2/signin";
  
  // If we are in overlay, we want the user redirected back to it after the
  // account is linked. We have to rely on HTTP_REFERER and do it manually
  // since the sign-in widget bypasses the overlay code when POSTing to
  // token_url, causing a complete page reload.
  //
  // Only clobber $_SESSION['hybridauth_overlay_uri'] if we haven't visited an account
  // deletetion confirm_form. Otherwise we will get a "Page not found" error
  // (@see http://drupal.org/node/1029458#comment-4055748), because HTTP_REFERER
  // has been changed to the confirmation form's URI.
  if (isset($_SESSION['hybridauth_account_deleted'])) {
    unset($_SESSION['hybridauth_account_deleted']);
  }
  else {
    unset($_SESSION['hybridauth_overlay_uri']);
    if (function_exists('overlay_get_mode') && overlay_get_mode() == 'child' && isset($_SERVER['HTTP_REFERER'])) {
      // Construct referer URI to be passed to token_url.
      $referer = $_SERVER['HTTP_REFERER'];
      if (variable_get('clean_url', FALSE)) {
        // Clean URLs are enabled.
        global $base_url;
        $referer = substr($referer, strlen($base_url)+1);
      }
      else {
        $referer = substr(parse_url($referer, PHP_URL_QUERY), 2);
      }
      global $user;
      $dest = urlencode('user/' . $user->uid . '/rpx');
      if (!variable_get('clean_url', FALSE)) {
        $dest = urlencode('?q=') . $dest;
      }
      $_SESSION['hybridauth_overlay_uri'] = $referer . '#overlay=' . $dest;
    }
  }
  */
  
  // TODO: Figure out a way to link a new account. Widget?
  /*
  $token_url = _hybridauth_token_url(array('add_to_account' => 'true'));
  
  $links['add_id'] = array(
    'title' => t('Add linked account'),
    'href' => $sign_in_url,
    'query' => array('token_url' => $token_url),
    'attributes' => array('class' => 'rpxnow', 'onclick' => 'return false;'),
  );
  
  $build['add_link'] = array(
    '#theme' => 'links',
    '#links' => $links,
    '#attributes' => array('class' => array('action-links')),
  );
  */
  
  $build['hybridauth_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('You don\'t have any accounts linked yet.')
  );
  
  /*
  if (isset($_SESSION['hybridauth_overlay_message'])) {
    drupal_set_message($_SESSION['hybridauth_overlay_message']['text'], $_SESSION['hybridauth_overlay_message']['type']);
    unset($_SESSION['hybridauth_overlay_message']);
  }
  */
  
  return $build;
}

/**
 * Menu callback; Delete the specified Engage identity from the system.
 */
function hybridauth_user_delete_form($form, $form_state, $account, $aid = NULL) {
  if (!$aid) {
    drupal_not_found();
    drupal_exit();
  }
  
  $provider_id = db_query("SELECT ha.provider_id FROM {authmap} am INNER JOIN {hybridauth_account} ha ON am.aid = ha.aid WHERE am.uid=:uid AND am.aid=:aid AND module=:module", array(
    ':uid' => $account->uid,
    ':aid' => $aid,
    ':module' => 'hybridauth',
  ))
  ->fetchField();
  
  if (!$provider_id) {
    drupal_not_found();
    drupal_exit();
  }
  
  $provider_name = hybridauth_get_provider_name($provider_id);
  $username = check_plain(format_username($account));
  
  return confirm_form(array(), t('Are you sure you want to remove this linked %provider account for %user?', array('%provider' => $provider_name, '%user' => $username)), 'user/' . $account->uid . '/hybridauth', '');
}

/**
 * Handle identity deletion by removing {authmap} and {hybridauth_linked_account}
 * entries.
 */
function hybridauth_user_delete_form_submit($form, &$form_state) {
  $uid = $form_state['build_info']['args'][0]->uid;
  $aid = $form_state['build_info']['args'][1];
  
  $linked_account = db_query("SELECT am.authname, am.provider_id FROM {authmap} am INNER JOIN {hybridauth_account} ha ON am.aid = ha.aid WHERE am.uid=:uid AND am.aid=:aid AND module=:module", array(
    ':uid' => $uid,
    ':aid' => $aid,
    ':module' => 'hybridauth',
  ))->fetchObject();

  // Linked account data has to be deleted from two tables, so use a transaction.
  $txn = db_transaction();
  try {
    $result1 = db_delete('authmap')
      ->condition('uid', $uid)
      ->condition('aid', $form_state['build_info']['args'][1])
      ->condition('module', 'hybridauth_core')
      ->execute();
    $result2 = db_delete('hybridauth_account')
      ->condition('aid', $form_state['build_info']['args'][1])
      ->execute();
  }
  catch (Exception $e) {
    $txn->rollback();
    watchdog_exception('hybridauth', $e);
  }
  
  if (isset($result1) && isset($result2)) {
    $provider_id = $linked_account->provider_id;
    $provider_name = hybridauth_get_provider_name($provider_id);
    
    drupal_set_message(t('The linked %provider account has been successfully removed.', array('%provider' => $provider_name)));
    // Save provider info for token replacement.
    $_SESSION['hybridauth_provider_info'] = array(
      'id' => $provider_id,
      'name' => $provider_name,
    );
    // Let other modules know that a linked account has been removed.
    $user = user_load($uid);
    $account = array(
      'user' => $user,
      'id' => _hybridauth_get_authmap_identfier($linked_account->authname),
      'provider_id' => $provider_id,
      'provider_name' => $provider_name,
    );
    module_invoke_all('hybridauth_account', 'delete', $account);
  }
  else {
    drupal_set_message(t('We were unable to delete the linked %provider account.', array('%provider' => $provider_name)), 'error');
  }

  // Let hybridauth_user_identities() know that the delete confirm_form has been
  // used (so that it can work around http://drupal.org/node/1029458#comment-4055748).
  $_SESSION['hybridauth_account_deleted'] = TRUE;

  $form_state['redirect'] = 'user/' . $uid . '/hybridauth';
}

