<?php
/**
 * @file
 * Contains functions for working with doi requests
 * Borrowed code from the islandora_doi module as we don't want
 * all the dependancies it would require.
 */

/**
 * Gets the XML as crossref associated with the supplied DOI.  Updates the
 * drupal form with the data.
 *
 * @param array $form
 *   The Drupal form
 * @param string $doi
 *   The DOI
 *
 * @return array
 *   A Drupal form with updated values.
 */
function upei_roblib_ill_doi_get_data($form, $doi) {
  if (empty($doi)) {
    $doi = 'Empty DOI';
  }
  $response = upei_roblib_ill_doi_load($doi);
  if ($response->data && strpos($response->data, "Malformed DOI") !== FALSE) {
    //try again without parsing the doi as sometimes parsing breaks it
    $response = upei_roblib_ill_doi_load($doi, FALSE);
  }
  $headers = array_change_key_case($response->headers);
  if ($response->data &&
    strpos($headers['content-type'], 'text/html') === FALSE &&
    strpos($response->data, "Malformed DOI") === FALSE
  ) {
    $crossref_xml = new DOMDocument('1.0');
    if (!$crossref_xml->loadXML($response->data)) {
      drupal_set_message(t('Error parseing DOI response, @msg'));
      return $form;
    }
    else {
      return upei_roblib_ill_populate_form_doi($crossref_xml, $form);
    }
  }
  else {
    drupal_set_message(t('Error retrieve data using DOI of @doi', ['@doi' => $doi]), 'error');
    return $form;
  }
}

/**
 * A convenience function for retrieving individual nodes from the Crossref XML
 * without throwing notices.
 *
 * @param DomDocument $crossref_xml
 *   The Crossref XML
 * @param string $node_name
 *   The node we want a value for, we are assuming the xml only has one node
 *   with the given name.
 * @param int $index
 *   If there are more than one element we can ask for the value of each
 *   element
 *
 * @return string
 *   Returns the value of the element
 */
function upei_roblib_ill_get_xml_node($crossref_xml, $node_name, $index = 0) {
  // TODO rewrite this function to use full xpath
  return empty($crossref_xml->getElementsbyTagName($node_name)
    ->item($index)) ? '' :
    $crossref_xml->getElementsbyTagName($node_name)->item($index)->nodeValue;
}


/**
 * Parse the response from Crossref and populate the request form
 *
 * @param DomDocument $crossref_xml
 *   The crossref xml
 * @param array $form
 *   Drupal form
 *
 * @return mixed
 *   An updated Drupal form
 */
function upei_roblib_ill_populate_form_doi($crossref_xml, $form) {
  $full_title = upei_roblib_ill_get_xml_node($crossref_xml, 'full_title');
  if (empty($full_title)) {
    $full_title = upei_roblib_ill_get_xml_node($crossref_xml, 'conference_name');
  }
  $form['ArticleTitle']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'title');
  if (empty($full_title)) {
    $form['Genre']['#value'] = 'book';
    $form['ArticleTitle']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'title', 1);
    if (isset($form['ArticleTitle']['#value'])) {
      $form['Genre']['#value'] = 'chapter';
    }
    $full_title = upei_roblib_ill_get_xml_node($crossref_xml, 'title', 0);
  }
  $form['Title']['#value'] = $full_title;
  $form['ISSN']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'issn');
  $form['Date']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'year');

  $form['ImageOrPageNumber']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'first_page');
  $form['PagesRequested']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'first_page') . '-' .
    upei_roblib_ill_get_xml_node($crossref_xml, 'last_page');
  $form['Volume']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'volume');
  $form['Issue']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'issue');
  $form['ISBN']['#value'] = upei_roblib_ill_get_xml_node($crossref_xml, 'isbn');
  foreach ($crossref_xml->getElementsbyTagName('person_name') as $person) {
    if ($person->getAttribute('contributor_role') == 'author' &&
      $person->getAttribute('sequence') == 'first'
    ) {
      $article_author = upei_roblib_ill_get_xml_node($person, 'surname') . ',' . upei_roblib_ill_get_xml_node($person, 'given_name');
    }
    if ($person->getAttribute('contributor_role') == 'editor' &&
      $person->getAttribute('sequence') == 'first'
    ) {
      $author = upei_roblib_ill_get_xml_node($person, 'surname') . ',' . upei_roblib_ill_get_xml_node($person, 'given_name');
    }
  }
  $form['ArticleAuthor']['#value'] = $article_author;
  $form['Author']['#value'] = $author;
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
 * @param bool parse
 *   parse the doi to remove prefix and whitespace etc.
 *
 * @return object
 *   An object as provided by drupal_http_request().
 */
function upei_roblib_ill_doi_load($doi, $parse = TRUE) {
  // Allows for $id to pass a DOI url string or the DOI name.
  $id = $parse ? upei_roblib_ill_doi_name_from_url($doi) : $doi;
  $openurl = variable_get('upei_roblib_ill_doi_openurl', 'http://www.crossref.org/openurl');
  $openurl_pid = variable_get('upei_roblib_ill_doi_openurl_pid', 'user@example.com');
  $url = url($openurl, [
    'query' => [
      'noredirect' => 'true',
      'pid' => $openurl_pid,
      'format' => 'unixref',
      'id' => ((strpos($id, 'doi:') === 0) ? $id : 'doi:' . $id),
    ],
  ]);
  return drupal_http_request($url);
}

