<?php

function hybridauth_endpoint() {
  // Make sure it's Drupal that starts the session.
  drupal_session_start();
  
  if ($lib_path = _hybridauth_library_path()) {
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

function hybridauth_providers($js) {
  $build = array(
    '#type' => 'hybridauth_widget',
    '#title' => '',
    '#hybridauth_widget_type' => 'list',
  );
  
  if ($js) {
    ctools_include('modal');
    ctools_modal_render(t('Log in using your account with'), $build);
  }
  else {
    $build['#title'] = t('Log in using your account with');
    return $build;
  }
}

function hybridauth_popup($provider_id) {
  // If provider is OpenID, but we don't have the OpenID target, show OpenID form.
  if ($provider_id == 'OpenID' && !isset($_GET['openid_identifier'])) {
    $form = drupal_get_form('hybridauth_openid_form');
    
    $page = array(
      'page_top' => '',
      '#children' => drupal_render($form),
      'page_bottom' => '',
    );
    print theme('html', array('page' => $page));
    drupal_exit();
  }
  
  // Make sure it's Drupal that starts the session then load the lib
  drupal_session_start();
  
  // Try to get HybridAuth instance
  try {
    $hybridauth = hybridauth_get_instance();
  }
  // PHP error
  catch (ErrorException $e) {
    drupal_set_message(t('There was an error processing your request!'), 'error');
    watchdog_exception('hybridauth', $e);
    // Cleanly close popup and redirect
    _hybridauth_popup_close();
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
    _hybridauth_popup_close($redirect);
  }
  
  return _hybridauth_popup_auth($hybridauth, $provider_id);
}

/**
 * Close the popup and redirect.
 */
function _hybridauth_popup_close($redirect = TRUE) {
  global $user;
  // Prevent devel module from spewing.
  $GLOBALS['devel_shutdown'] = FALSE;
  //visitors can register accounts without admin approval
  //if ((!variable_get('hybridauth_register', 0) && variable_get('user_register', USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL) == USER_REGISTER_VISITORS)
  //  || variable_get('hybridauth_register', 0) == 1) {
    $destination = drupal_get_destination();
    $destination = token_replace($destination['destination'], array('user' => $user), array('clear' => TRUE));
  //}
  //redirect to front page
  //else {
  //  $destination = '';
  //}
  drupal_add_js('
    var redirect = ' . ($redirect ? 'true' : 'false') . ';
    if (window.opener && redirect){
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
    $profile['provider'] = $provider_id;
  }
  catch(Exception $e) {
    $_SESSION['hybridauth']['stat'] = $e->getMessage();
    drupal_set_message(t('We were unable to complete your request.'), 'error');
    watchdog_exception('hybridauth', $e);
    
    // Cleanly close popup and redirect
    _hybridauth_popup_close();
  }
  
  // Process Drupal authentication.
  _hybridauth_popup_process_auth($profile);
}

/**
 * A helper function that takes a successful HA authentication and handles the Drupal side of things.
 */
function _hybridauth_popup_process_auth($data) {
  global $user;
  
  //user is already logged in, tries to add new identity
  if (user_is_logged_in()) {
    //identity is already registered
    if ($identity = _hybridauth_identity_load($data)) {
      //registered to this user
      if ($user->uid == $identity['uid']) {
        drupal_set_message(t('You have already registered this identity.'));
        _hybridauth_popup_close();
      }
      //registered to another user
      else {
        drupal_set_message(t('This identity is registered to another user.'), 'error');
        _hybridauth_popup_close();
      }
    }
    //identity is not registered - register it to the logged in user
    else {
      _hybridauth_identity_save($data);
      drupal_set_message(t('New identity added.'));
      //invoke hybridauth_identity_added rules event
      if (module_exists('rules')) {
        rules_invoke_event('hybridauth_identity_added', $user, $data);
      }
      _hybridauth_popup_close();
    }
  }
  
  if ($identity = _hybridauth_identity_load($data)) {
    //check if user is blocked
    if ($account = _hybridauth_user_is_blocked_by_uid($identity['uid'])) {
      drupal_set_message(t('The username %name has not been activated or is blocked.', array('%name' => $account->name)), 'error');
    }
    elseif (!_hybridauth_user_login_access_by_uid($identity['uid'])) {
      drupal_set_message(t('You need to verify your e-mail address - !email.', array('!email' => $data['email'])), 'error');
      drupal_set_message(t('A welcome message with further instructions has been sent to your e-mail address.'));
      _user_mail_notify('register_no_approval_required', user_load($identity['uid']));
    }
    else {
      $form_state['uid'] = $identity['uid'];
      user_login_submit(array(), $form_state);
    }
  }
  //handle duplicate email addresses
  elseif (variable_get('hybridauth_duplicate_emails', 1) && !empty($data['email']) && $account = user_load_by_mail($data['email'])) {
    //block registration
    if (variable_get('hybridauth_duplicate_emails', 1) == 1) {
      drupal_set_message(t('You are trying to login with email address of another user.'), 'error');
      if (!empty($account->data['hybridauth'])) {
        $providers = hybridauth_providers_list();
        drupal_set_message(t('If you are completely sure it is your email address, try to login through %provider.',
          array('%provider' => $providers[$account->data['hybridauth']['provider']])), 'status');
      }
      else {
        drupal_set_message(t('If you are completely sure it is your email address, try to login using your username and password on this site. If you don\'t remember your password - <a href="@password">request new password</a>.',
          array('@password' => url('user/password'))));
      }
    }
    //add identity to existing account, only if emailVerified
    elseif (variable_get('hybridauth_duplicate_emails', 1) == 2 && $data['email'] == $data['emailVerified']) {
      _hybridauth_identity_save($data, $account->uid);
      drupal_set_message(t('New identity added.'));
      //invoke hybridauth_identity_added rules event
      if (module_exists('rules')) {
        rules_invoke_event('hybridauth_identity_added', $account, $data);
      }
      $form_state['uid'] = $account->uid;
      user_login_submit(array(), $form_state);
    }
  }
  //create new user account
  else {
    // Check profile information for required fields.
    _hybridauth_check_required_fields($data);
    global $hybridauth_data;
    $hybridauth_data = $data;
    //visitors can create accounts
    if ((!variable_get('hybridauth_register', 0) && variable_get('user_register', USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL))
      || variable_get('hybridauth_register', 0)) {
      user_external_login_register(_hybridauth_make_username($data), 'hybridauth');
      _hybridauth_user_save($data);
    }
    //only admin can create accounts
    else {
      drupal_set_message(t('Only site administrators can create new user accounts.'), 'error');
    }
  }
  
  _hybridauth_popup_close();
}

function _hybridauth_check_required_fields($data) {
  $required_fields = array_filter(variable_get('hybridauth_required_fields', array('email' => 'email')));
  foreach ($required_fields as $key => $value) {
    if (empty($data[$key]) && !($data[$key] === 0)) {
      $form = drupal_get_form('hybridauth_required_fields_form', $data);

      $page = element_info('page');
      $page['#children'] = theme('status_messages') . drupal_render($form);

      print theme('html', array('page' => $page));
      drupal_exit();
    }
  }
}

function hybridauth_required_fields_form($form, &$form_state, $data) {
  $hybridauth_fields = hybridauth_fields_list();
  $required_fields = array_filter(variable_get('hybridauth_required_fields', array('email' => 'email')));
  $form = array();
  $form['#data'] = $data;
  
  foreach ($required_fields as $key => $value) {
    if (empty($data[$key]) && !($data[$key] === 0)) {
      $form[$key] = array(
        '#type' => 'textfield',
        '#title' => $hybridauth_fields[$key],
        '#required' => TRUE,
      );
      if ($key == 'email') {
        $form[$key]['#maxlength'] = EMAIL_MAX_LENGTH;
        $form[$key]['#description'] = t('A valid e-mail address. All e-mails from the system will be sent to this address. The e-mail address is not made public and will only be used if you wish to receive a new password or wish to receive certain news or notifications by e-mail.');
      }
      if ($key == 'gender') {
        $form[$key]['#type'] = 'radios';
        $form[$key]['#options'] = array(
          'male' => t('Male'),
          'female' => t('Female'),
        );
      }
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

function hybridauth_required_fields_form_validate($form, &$form_state) {
  // Trim whitespace from mail, to prevent confusing 'e-mail not valid'
  // warnings often caused by cutting and pasting.
  $mail = trim($form_state['values']['email']);
  form_set_value($form['email'], $mail, $form_state);

  // Validate the e-mail address.
  if ($error = user_validate_mail($form_state['values']['email'])) {
    form_set_error('email', $error);
  }
}

function hybridauth_required_fields_form_submit($form, &$form_state) {
  $data = $form['#data'];
  $required_fields = array_filter(variable_get('hybridauth_required_fields', array('email' => 'email')));
  
  foreach ($required_fields as $key => $value) {
    if (empty($data[$key]) && !($data[$key] === 0)) {
      $data[$key] = $form_state['values'][$key];
    }
  }

  _hybridauth_popup_process_auth($data);
}

function hybridauth_openid_form($form, &$form_state) {
  $form = array();
  
  $form['openid_identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('OpenID Identity'),
    '#description' => t('Type your OpenID identity you want to use.'),
  );
  
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  
  return $form;
}

function hybridauth_openid_form_submit($form, &$form_state) {
  $query = drupal_get_query_parameters() + drupal_get_destination();
  unset($_GET['destination']);
  $query['openid_identifier'] = $form_state['values']['openid_identifier'];
  
  drupal_goto('hybridauth/popup/OpenID', array('query' => $query));
}

/**
 * Menu callback; manage Engage 3rd party identities for the specified user.
 */
function hybridauth_user_identity($form, &$form_state, $account) {
  drupal_set_title(format_username($account));
  
  $identities = _hybridauth_identity_load_by_uid($account->uid);
  $providers = hybridauth_providers_list();
  
  $header = array(t('Authentication provider'), t('Identity'), t('Delete'));
  $rows = array();
  $data_array = array();
  foreach ($identities as $identity) {
    $data = unserialize($identity['data']);
    $data_array[] = $data;
    $rows[] = array(
      $providers[$data['provider']],
      l($data['profileURL'], $data['profileURL'], array('attributes' => array('target' => '_blank'), 'external' => TRUE)),
      l(t('Delete'), 'user/' . $account->uid . '/hybridauth/delete/' . $identity['id']),
    );
  }
  
  $form = array();
  
  $form['identity'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('You don\'t have any identities yet.')
  );
  
  //add more identities
  if (user_access('use hybridauth')) {
    $form['hybridauth_widget'] = array(
      '#type' => 'hybridauth_widget',
      '#title' => t('Add more identities'),
      '#weight' => 10,
      '#hybridauth_widget_type' =>'list',
      '#hybridauth_destination' => '',
    );
  }
  
  //tokens browser for admins
  if (user_access('administer site configuration') || user_access('administer users')) {
    $form['vtabs'] = array(
      '#type' => 'vertical_tabs',
      '#weight' => 20,
    );
    
    $header = array(t('Token'), t('Value'));
    //user tokens
    if (!empty($account->data['hybridauth'])) {
      $form['vtabs']['fset_user_tokens'] = array(
        '#type' => 'fieldset',
        '#title' => t('User tokens'),
      );
      
      $rows = array();
      foreach ($account->data['hybridauth'] as $key => $value) {
        $rows[] = array('[user:hybridauth:' . $key . ']', $value);
      }
      $form['vtabs']['fset_user_tokens']['tokens'] = array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows,
      );
    }
    
    //data from auth providers
    foreach ($data_array as $data) {
      $form['vtabs']['fset_' . $data['provider'] . '_' . $data['identifier']] = array(
        '#type' => 'fieldset',
        '#title' => $providers[$data['provider']],
      );
      
      $rows = array();
      foreach ($data as $key => $value) {
        $rows[] = array($key, $value);
      }
      $form['vtabs']['fset_' . $data['provider'] . '_' . $data['identifier']]['tokens'] = array(
        '#theme' => 'table',
        '#header' => $header,
        '#rows' => $rows,
      );
    }
  }
  
  return $form;
}

function hybridauth_user_identity_delete($form, &$form_state, $account, $id) {
  $del_identity = _hybridauth_identity_load_by_id($id);
  if (!$del_identity || $account->uid != $del_identity['uid']) {
    drupal_set_message(t('You are trying to delete non-existing identity.'), 'error');
    drupal_access_denied();
  }
  $del_identity_data = unserialize($del_identity['data']);
  $username = format_username($account);
  $question = t('Are you sure you want to detach the HybridAuth identity !identity from %user?',
    array(
      '!identity' => l($del_identity_data['profileURL'], $del_identity_data['profileURL'], array('attributes' => array('target' => '_blank'), 'external' => TRUE)),
      '%user' => $username));
  
  $form = array();
  $form['#user'] = $account;
  $form['#del_identity'] = $del_identity;
  $form['#del_identity_data'] = $del_identity_data;
  $form['question'] = array(
    '#markup' => $question,
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  if (!empty($account->data['hybridauth']) && $account->data['hybridauth']['provider'] == $del_identity_data['provider'] &&
    $account->data['hybridauth']['identifier'] == $del_identity_data['identifier']) {
    $identities = _hybridauth_identity_load_by_uid($account->uid);
    $providers = hybridauth_providers_list();
    $options = array();
    foreach ($identities as $key => $identity) {
      $data = unserialize($identity['data']);
      if ($key != $id) {
        $options[$key] = $providers[$identity['provider']] . ' - ' . l($data['profileURL'], $data['profileURL'], array('attributes' => array('target' => '_blank'), 'external' => TRUE));
      }
    }
    
    if (!empty($options)) {
      $form['explanation'] = array(
        '#markup' => t('This identity was used to create your account. Please choose another identity to replace it.'),
        '#prefix' => '<div>',
        '#suffix' => '</div>',
      );
      $form['identity_choice'] = array(
        '#type' => 'radios',
        //'#title' => t('Identities'),
        '#options' => $options,
        '#default_value' => count($options) == 1 ? $key : NULL,
        //'#required' => TRUE, //required has bugs with radios http://drupal.org/node/811542
      );
    }
    else {
      $form['explanation'] = array(
        '#markup' => t('This identity was used to create your account. To delete it you should first add another identity to your account.'),
        '#prefix' => '<div>',
        '#suffix' => '</div>',
      );
      //add more identities
      if (user_access('use hybridauth')) {
        $form['hybridauth_widget'] = array(
          '#type' => 'hybridauth_widget',
          '#title' => t('Add more identities'),
          '#weight' => 10,
          '#hybridauth_widget_type' =>'list',
          '#hybridauth_destination' => '',
        );
      }
      return $form;
    }
  }
  
  $form = confirm_form($form, '', 'user/' . $account->uid . '/hybridauth');
  drupal_set_title($username);
  
  return $form;
}

function hybridauth_user_identity_delete_validate($form, &$form_state) {
  if (!empty($form['identity_choice']) && empty($form_state['values']['identity_choice'])) {
    form_set_error('identity_choice', t('Please choose identity for replacement.'));
  }
}

function hybridauth_user_identity_delete_submit($form, &$form_state) {
  $account = $form['#user'];
  $del_identity = $form['#del_identity'];
  $del_identity_data = $form['#del_identity_data'];
  if (!empty($form_state['values']['identity_choice'])) {
    $identity = _hybridauth_identity_load_by_id($form_state['values']['identity_choice']);
    $data = unserialize($identity['data']);
    $name = _hybridauth_make_username($data);
    //change name
    $edit['name'] = $name;
    //change hybridauth data used for tokens
    $edit['data']['hybridauth'] = $data;
    $account = user_save($account, $edit);
    //change authname in authmap DB table
    user_set_authmaps($account, array('authname_hybridauth' => $name));
  }
  
  $deleted = _hybridauth_identity_delete_by_id($del_identity['id']);
  if ($deleted) {
    drupal_set_message(t('Identity deleted.'));
    //invoke hybridauth_identity_deleted rules event
    if (module_exists('rules')) {
      rules_invoke_event('hybridauth_identity_deleted', $account, $del_identity_data);
    }
  }
  
  $form_state['redirect'] = 'user/' . $account->uid . '/hybridauth';
}
