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
 * @param array $form_state
 *   A drupal form_state array
 * @param string $aid
 *   A relais authentication id (token)
 */
function upei_roblib_ill_add_request($form_state, $aid) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/db');
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $url = variable_get('upei_roblib_ill_add_url', 'https://caul-cbua.relais-host.com/portal-service/request/add');
  $relais_arr = array(
    "SupplyingLibrarySymbol" => variable_get('upei_roblib_ill_library_symbol','PCU'),
  );
  $relais_arr['BibliographicInfo'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $relais_arr['DeliveryAddress'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_auth_form']);
  $request_info = upei_roblib_ill_request_info_array($relais_arr['BibliographicInfo'], $form_state['values']['notes']);
  $relais_arr['RequestInfo'] = $request_info;
  $relais_arr['PublisherInfo']['PublicationDate'] = isset($form_state['storage']['upei_roblib_ill_request_form']['Date']) ?
    $form_state['storage']['upei_roblib_ill_request_form']['Date'] : '';
  $relais_arr['PublisherInfo']['PublicationType'] = upei_roblib_ill_get_pub_type($form_state['storage']['upei_roblib_ill_request_form']['Genre']);
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
  upei_roblib_ill_log_request($relais_arr, $response_arr);
  return $response_arr;
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
    if (isset($response_data['Problem']) || $response_data['LastName'] !== $surname) {
      $err_message = isset($response_data['Problem']['Message']) ? $response_data['Problem']['Message'] : '';
      $err_message .= t( '  Your Campus ID could be incorrect or your Last Name does not match what is stored in the profile for the user with the specified Campus ID');
      $response_data['Problem']['Message'] = $err_message;
      return $response_data;
    }
    $aid = $response_data['AuthorizationId'];
  }
  return isset($aid) ? $aid : $response_data;
}

