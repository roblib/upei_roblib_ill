<?php
/**
 * @file
 * Contains functions for working with doi requests
 * Borrowed code from the islandora_doi module as we don't want
 * all the dependancies it would require.
 */


function upei_roblib_ill_doi_get_data($form, $doi) {
  if (empty($doi)) {
    $doi = 'Empty DOI';
  }
  $response = upei_roblib_ill_doi_load($doi);
  $headers = array_change_key_case($response->headers);
  if ($response->data &&
    strpos($headers['content-type'], 'text/html') === FALSE &&
    strpos($response->data, "Malformed DOI") === FALSE
  ) {
    $crossref_xml = new DOMDocument('1.0');
    if (!$crossref_xml->loadXML($response->data)) {
      drupal_set_message(t('Error parseing DOI response, @msg'));
      $form['doi']['#value'] = '';
      return $form;
    }
    else {
      return upei_roblib_ill_populate_form_doi($crossref_xml, $form);
    }
  }
  else {
    drupal_set_message(t('Error retrieve data using DOI of @doi', array('@doi' => $doi)), 'error');
    $form['doi']['#value'] = '';
    return $form;
  }
}

function upei_roblib_ill_get_xml_node($crossref_xml, $element_name) {
  return  empty($crossref_xml->getElementsbyTagName($element_name)->item(0)) ? '' :
    $crossref_xml->getElementsbyTagName($element_name)->item(0)->nodeValue;
}


/**
 * Parse the response from Crossref and populate the request form
 * @param DomDocument $crossref_xml
 *   The crossref xml
 * @param array $form
 *   Drupal form
 * @return mixed
 *   A Drupal form
 */
function upei_roblib_ill_populate_form_doi($crossref_xml, $form) {
  $full_title = upei_roblib_ill_get_xml_node($crossref_xml, 'full_title');
  if (empty($full_title)) {
    $full_title = upei_roblib_ill_get_xml_node($crossref_xml, 'conference_name');
  }
  $form['Title']['#value'] = $full_title;
  $form['ISSN']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'issn');
  $form['Date']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'year');
  $form['ArticleTitle']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'title');
  $form['ImageOrPageNumber']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'first_page');
  $form['PagesRequested']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'last_page');
  $form['Volume']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'volume');
  $form['Issue']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'issue');
  foreach ($crossref_xml->getElementsbyTagName('person_name') as $person) {
    $role = $person->getAttribute('contributor_role');
    $sequence = $person->getAttribute('sequence');
    if ($person->getAttribute('contributor_role') == 'author' &&
      $person->getAttribute('sequence') == 'first'
    ) {
      $author = upei_roblib_ill_get_xml_node($crossref_xml, 'surname') . ',' . upei_roblib_ill_get_xml_node($crossref_xml, 'given_name');
    }
  }
  $form['ArticleAuthor']['#value'] = $author;
  $form['doi']['#value'] = '';
  return $form;
}

/**
 * Get the DOI name for the given url.
 *
 * @param string $doi_url
 *   A DOI url, or a DOI name with or without the "doi:" prefix. If the $url
 *   is a CrossRef ID passed in as doi:10.1016/j.tiv.2011.10.017 or
 *   10.1111/eve.12339 it will not get changed by running it through parse_url.
 *
 * @return string
 *   A string containing the DOI name.
 */
function upei_roblib_ill_doi_name_from_url($doi_url) {
  // Allows for DOI to be entered in the following formats: 10.1111/eva.12339,
  // http://dx.doi.org/10.1111/eva.12339, http://doi.org/10.1111/eva.12339,
  // or doi:10.1111/eva.12339. Removing whitespace before parsing the url and
  // then removing any trailing "/" from the returned path.
  $doi_name = trim(parse_url(trim($doi_url), PHP_URL_PATH), '/');
  return $doi_name;
}


/**
 * Perform a request to CrossRef for the given ID.
 *
 * @param string $doi
 *   A DOI to lookop. May be provided with or without the "doi:" prefix. Can
 *   also pass a full DOI url (i.e, http://doi.org/10.1111/eva.12339 ).
 *
 * @return object
 *   An object as provided by drupal_http_request().
 */
function upei_roblib_ill_doi_load($doi) {
  // Allows for $id to pass a DOI url string or the DOI name.
  $id = upei_roblib_ill_doi_name_from_url($doi);
  $openurl = variable_get('upei_roblib_ill_doi_openurl', 'http://www.crossref.org/openurl');
  $openurl_pid = variable_get('upei_roblib_ill_doi_openurl_pid', 'user@example.com');
  $url = url($openurl, array(
    'query' => array(
      'noredirect' => 'true',
      'pid' => $openurl_pid,
      'format' => 'unixref',
      'id' => ((strpos($id, 'doi:') === 0) ? $id : 'doi:' . $id),
    ),
  ));
  return drupal_http_request($url);
}

