<?php
/**
 * @file
 * Contains helper functions for working with Relais requests.
 */

/**
 * Clean the form content before sending it to relais or logging it.
 *
 * @param $value
 * @param $key
 */
function upei_roblib_ill_check_arr_item(&$value, $key) {
  if (is_array($value)) {
    array_walk($value, 'upei_roblib_ill_check_arr_item');
  }
  else {
    $value = filter_xss($value);
  }
}

/**
 * Removes empty elements and drupal specific elements from an array
 *
 * @param $values
 *   A drupal form_state['values] array
 *
 * @return array
 *   An array where the keys in the array that have empty values are removed and
 * drupal specific elements are removed.
 */
function upei_roblib_ill_clean_array($values) {
  $arr = array_filter($values);
  if (isset($arr['ISSN'])) {
    $arr['ISSN'] = [$arr['ISSN']];
  }
  if (isset($arr['ISBN'])) {
    $arr['ISBN'] = [$arr['ISBN']];
  }
  // The below checks are due to ebscos habit of sending sending the same info in both atitle and title etc.
  if (isset($arr['Genre']) && $arr['Genre'] == 'article' && isset($arr['Author'])) {
    // We want ArticleAuthor
    unset($arr['Author']);
  }
  if (isset($arr['Genre']) && $arr['Genre'] == 'book' && isset($arr['ArticleAuthor'])) {
    // We want Author
    unset($arr['ArticleAuthor']);
  }
  if (isset($arr['Genre']) && $arr['Genre'] == 'book' && isset($arr['ArticleTitle'])) {
    // We want Title only
    unset($arr['ArticleTitle']);
  }
  if (isset($arr['doi'])) {
    $arr['AdditionalNumbers'] = $arr['doi'];
  }
  unset($arr['form_build_id']);
  unset($arr['form_token']);
  unset($arr['form_id']);
  unset($arr['op']);
  unset($arr['submit']);
  unset($arr['next']);
  unset($arr['certify']);
  unset($arr['doi']);
  unset($arr['doi_button']);
  unset($arr['honeypot_time']);
  array_walk($arr, 'upei_roblib_ill_check_arr_item');
  return $arr;
}

/**
 * Allows us to ask for _REQUEST variables that may or may not exist without
 * generating a php warning. Also does some formatting for certain variables.
 *
 * @param string $variable
 *   The name of the $_REQUEST variable to check
 *
 * @return string
 *   The value of the $_REQUEST variable or an empty string.
 */
function upei_roblib_ill_get_request_variable($variable) {
  if ($variable == 'author' && empty($_REQUEST[$variable])) {
    //google scholar usually sends auinit aulast instead of author
    $initial = isset($_REQUEST['auinit']) ? check_plain($_REQUEST['auinit']) : NULL;
    $last_name = isset($_REQUEST['aulast']) ? check_plain($_REQUEST['aulast']) : NULL;
    return !empty($last_name) ? $last_name . ',' . $initial : '';
  }
  if ($variable == 'issn' && !empty($_REQUEST[$variable])) {
    // ebsco sometimes sends garbage as issns verify this is a valid issn before displaying the value in the form.
    return preg_match('/^\d{4}-?\d{3}[\dxX]$/', $_REQUEST[$variable]) ? check_plain($_REQUEST[$variable]) : '';
  }
  // ebsco sometimes returns bookitem we only understand chapter
  if (($variable == 'genre' && !empty($_REQUEST[$variable])) && $_REQUEST[$variable] == 'bookitem') {
    return 'chapter';
  }
  return isset($_REQUEST[$variable]) ? check_plain($_REQUEST[$variable]) : '';
}


function upei_roblib_ill_get_doi_from_request() {
  return (!empty($_REQUEST['doi'])) ? $_REQUEST['doi'] : upei_roblib_ill_get_doi_from_id();
}

function upei_roblib_ill_get_doi_from_id() {
  if (!empty($_REQUEST['ID'])) {
    return (strpos($_REQUEST['ID'], 'doi:') !== 0) ? '' : $_REQUEST['ID'];
  }
  return '';
}

/**
 * Populates the requestInfo portion of a Relais request
 *
 * @param array $values
 *   an array containing a 'Genre' element.
 *
 * @return array
 *   The requestInfo portion of a Relais request
 */
function upei_roblib_ill_request_info_array($values, $notes) {
  $requestInfo = [];
  $genre = $values['Genre'];
  $requestInfo['DateSubmitted'] = date('Y-m-d H:i:s');
  $requestInfo['Notes'] = $notes;
  if ($genre == 'book') {
    $requestInfo['ServiceType'] = 'L';
    $requestInfo['ServiceLabel'] = "R";
    //$requestInfo['RequestSource'] = "C";
  }
  else {
    $requestInfo['ServiceType'] = 'X';
    $requestInfo['ServiceLabel'] = "R";
    //$requestInfo['RequestSource'] = "C";
  }
  return $requestInfo;
}

/**
 * Creates a summary Table based on the biblio information in the form_state
 * array for display
 *
 * @param $form_state
 *
 * @return string
 */
function upei_roblib_format_biblio_info($form_state) {
  $data = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $rows = [];
  foreach ($data as $key => $value) {
    if ($key == 'ISSN' || $key == 'ISBN') {
      $value = reset($value);
    }
    array_push($rows, [$key, $value]);
  }
  $output = [
    '#theme' => 'table',
    '#rows' => $rows,
    '#prefix' => '<div class=upei-roblib-ill-biblio-info>',
    '#suffix' => '</div>',
  ];
  $html_output = drupal_render($output);
  return $html_output;
}
