<?php
/**
 * @file
 * Contains the functions to send and receive data to/from Relais.
 */

function upei_roblib_ill_get_pub_type($genre) {
     switch ($genre) {
       case 'book' :
         return 'B';
       case 'chapter' :
         return 'I';
       default :
         return 'J';
     }
}

/**
 * Submit an ILL AddRequest to relais.
 *
 * @param array $form_state
 *   A drupal form_state array
 * @param string $aid
 *   A relais authentication id (token)
 * @return array|mixed
 */
function upei_roblib_ill_add_request($form_state, $aid) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/db');
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $url = variable_get('upei_roblib_ill_add_url', 'https://caul-cbua.relais-host.com/portal-service/request/add');
  $relais_arr = upei_roblib_ill_build_relais_arr($form_state);
  $relais_json = json_encode($relais_arr);
  $options = array(
    'method' => 'POST',
    'data' => $relais_json,
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );
  if(!isset($aid) || is_array($aid)) {
    upei_roblib_ill_log_request($relais_arr, $aid);
    drupal_set_message(t('Error retrieving authentication token, @message', array('@message' => $aid['Problem']['Message'])), 'error');
    return array('ConfirmMessage' =>  t('There was an error processing your request, @msg', array('@msg' => $aid['Problem']['Message'])));
  }
  $result = drupal_http_request($url . '?aid=' . $aid, $options);
  module_load_include('inc', 'upei_roblib_ill', 'includes/db');
  if(!isset($result->data)) {
    return array('ConfirmMessage' => 'Error Communicating with Relais, no data returned.');
  }
  $response_json = $result->data;
  $response_arr = json_decode($response_json, TRUE);
  $response_arr['ConfirmMessage'] = isset($response_arr['Problem']['ErrorMessage']) ? $response_arr['Problem']['ErrorMessage'] : $response_arr['ConfirmMessage'];
  upei_roblib_ill_log_request($relais_arr, $response_arr);
  return $response_arr;
}

/**
 * @param $form_state
 * @return array
 */
function upei_roblib_ill_build_relais_arr($form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $relais_arr = array(
    "SupplyingLibrarySymbol" => variable_get('upei_roblib_ill_library_symbol','PCU'),
  );
  $pub_type = upei_roblib_ill_get_pub_type($form_state['storage']['upei_roblib_ill_request_form']['Genre']);
  $relais_arr['BibliographicInfo'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $relais_arr['DeliveryAddress'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_auth_form']);
  if($pub_type != 'B') {
    // this is not a book but is a journal or chapter we will send electronically and as copy
    $relais_arr['ElectronicDelivery']['DeliveryAddress'] = $relais_arr['ElectronicDelivery']['MessagingAddress'] =
      $relais_arr['DeliveryAddress']['DeliveryAddress'];
  }
  $request_info = upei_roblib_ill_request_info_array($relais_arr['BibliographicInfo'], $form_state['values']['notes']);
  $relais_arr['RequestInfo'] = $request_info;
  $relais_arr['PublisherInfo']['PublicationDate'] = isset($form_state['storage']['upei_roblib_ill_request_form']['Date']) ?
    $form_state['storage']['upei_roblib_ill_request_form']['Date'] : '';
  $relais_arr['PublisherInfo']['PublicationType'] = $pub_type;
  return $relais_arr;
}

/**
 * Request an aid from Relais.
 * @param string $barcode
 *   A patron_id, campus_id or barcode to identify a user
 * @return string/array
 *   Returns a Relais authentication id (token) or an array containing the Relais response error which should include the error message if authentication fails
 */
function upei_roblib_ill_authenticate($barcode, $surname) {
  $url = variable_get('upei_roblib_ill_auth_url', 'https://caul-cbua.relais-host.com/portal-service/user/authentication');
  $json_arr = array();
  $json_arr['ApiKey'] = variable_get('upei_roblib_ill_relais_key');
  $json_arr['UserGroup'] = 'patron';
  $json_arr['LibrarySymbol'] = variable_get('upei_roblib_ill_library_symbol');
  $json_arr['PatronId'] = $barcode;
  $request_json = json_encode($json_arr);
  $options = array(
    'method' => 'POST',
    'data' => $request_json,
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );
  $result = drupal_http_request($url, $options);
  if($result->code != '200') {
    $response_data['Problem']['Message'] = $result->error;
    return $response_data;
  }
  $response_data = json_decode($result->data, TRUE);
  if ($result->code == '200') {
    if (isset($response_data['Problem']) || strtolower($response_data['LastName']) !== strtolower($surname)) {
      $err_message = isset($response_data['Problem']['Message']) ? $response_data['Problem']['Message'] : '';
      $response_data['Problem']['Message'] = upei_roblib_ill_build_err_msg($err_message);
      return $response_data;
    }
    $aid = $response_data['AuthorizationId'];
  }
  return isset($aid) ? $aid : $response_data;
}

function upei_roblib_ill_build_err_msg($msg_from_server) {
  $campus_id = l(t('Campus ID'), 'http://www.upei.ca/vpaf/campuscard');
  $ill_email = l(t('ill@upei.ca'), 'mailto:ill@upei.ca');
  $phone = l(t('902-566-0583'), 'tel:902-566-0353');
  return t('Oops. Something went wrong.<br />Check the "Your Last Name" and "Your Campus ID" fields - those two need to match what is on file
. (Your !campus_id appears as the NUMBER near the middle of your campus card).  If you do not have a !campus_id, please contact the Robertson Library Service Desk, or, 
call !phone.  Note: if you are a student taking online courses only, please email !ill_email.<br />Server response: @msg_from_server',
    array('!campus_id' => $campus_id, '!ill_email' => $ill_email, '!phone' => $phone, '@msg_from_server' => $msg_from_server));
}

