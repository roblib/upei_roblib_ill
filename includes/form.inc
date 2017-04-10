<?php
/**
 * @file
 * Contains the ILL forms.
 */

/**
 * A form to wrap the different steps of our multi step form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_form($form, &$form_state) {
  if (!isset($form_state['values'])) {
    $form_name = 'upei_roblib_ill_request_form';
  }
  else {
    $form_name = 'upei_roblib_ill_auth_form';
  }
  $form_state['step'] = $form_name;
  return $form_name === 'upei_roblib_ill_request_form' ? upei_roblib_ill_request_form($form, $form_state) :
    upei_roblib_ill_auth_form($form, $form_state);
}


/**
 * Validation for the upei_roblib_ill_form form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function upei_roblib_ill_form_validate($form, &$form_state) {
  if ($form_state['step'] == 'upei_roblib_ill_request_form' && empty($form_state['values']['doi']) && empty($form_state['values']['Title']) &&
    empty($form_state['values']['ArticleTitle'])
  ) {
    form_set_error('Title', t('One of "DOI", "Title" or "Article Title" values must be provided'));
  }

}

/**
 * Submit handler for the upei_roblib_ill_form form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function upei_roblib_ill_form_submit($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/relais');
  $form_state['storage'][$form_state['step']] = $form_state['values'];
  switch ($form_state['step']) {
    case 'upei_roblib_ill_request_form':
      if ($form_state['clicked_button']['#value'] != 'lookup DOI') {
        $form_state['step'] = 'upei_roblib_ill_auth_form';
        $form_state['rebuild'] = TRUE;
      }
      break;
    case 'upei_roblib_ill_auth_form':
      //process the form
      drupal_set_message('completed form');
      $aid = upei_roblib_ill_authenticate($form_state['values']['campus_id']);
      upei_roblib_ill_add_request($form_state, $aid);
      break;
  }

}


/**
 * The patron portion of the ILL form.
 *
 * The campus_id and email portion are also used in a request for an Relais aid.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_auth_form($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $form['#prefix'] = upei_roblib_format_biblio_info($form_state);

  $form['FirstName'] = array(
    '#type' => 'textfield',
    '#title' => t('Your First Name'),
    '#size' => 50,
    '#required' => TRUE,
  );
  $form['Surname'] = array(
    '#type' => 'textfield',
    '#title' => t('Your Last Name'),
    '#size' => 50,
    '#required' => TRUE,
  );
  $form['campus_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Your Campus ID'),
    '#size' => 50,
    '#required' => TRUE,
  );
  $patron_types = array(
    'None' => t('None'),
    'UPEI Faculty/Staff' => t('UPEI Faculty/Staff'),
    'UPEI Graduate Student' => t('UPEI Graduate Student'),
    'UPEI Undergraduate Student' => t('UPEI Undergraduate Student'),
    'UPEI Honours Undergraduate Student' => t('UPEI Honours Undergraduate Student'),
    'UPEI Alumni Premiere member' => t('UPEI Alumni Premiere member'),
    'UPEI Alumni member' => t('UPEI Alumni member'),
    'Public Borrower' => t('Public Borrower'),
  );
  $departments = array(
    'Applied Human Sciences' => t('Applied Human Sciences'),
    'AVC - Biomedical Sciences' => t('AVC - Biomedical Sciences'),
    'AVC - Companion Animals' => t('AVC - Companion Animals'),
    'AVC - Health Management' => t('AVC - Health Management'),
    'AVC - Pathology/Microbiology' => t('AVC - Pathology/Microbiology'),
    'AVC - Lobster Science' => t('AVC - Lobster Science'),
    'Biology' => t('Biology'),
    'Business' => t('Business'),
    'Chemistry' => t('Chemistry'),
    'Classics' => t('Classics'),
    'Diversity and Social Justice Studies' => t('Diversity and Social Justice Studies'),
    'Economics' => t('Economics'),
    'Education' => t('Education'),
    'English' => t('English'),
    'Engineering' => t('Engineering'),
    'Environmental Studies' => t('Environmental Studies'),
    'Fine Arts' => t('Fine Arts'),
    'History' => t('History'),
    'Island Studies' => t('Island Studies'),
    'Island Studies (Masters)' => t('Island Studies (Masters)'),
    'Robertson Library' => t('Robertson Library'),
    'Robertson Library' => t('Robertson Library'),
    'Mathematical and Computational Sciences' => t('Mathematical and Computational Sciences'),
    'Modern Languages' => t('Modern Languages'),
    'Music' => t('Music'),
    'Nursing' => t('Nursing'),
    'Philosophy' => t('Philosophy'),
    'Physics' => t('Physics'),
    'Political Science' => t('Political Science'),
    'Psychology' => t('Psychology'),
    'Religious Studies' => t('Religious Studies'),
    'Sociology/Anthropology' => t('Sociology/Anthropology'),
    'Other' => t('Other (Please add to the Notes field)'),
  );
  $form['patron_type'] = array(
    '#type' => 'select',
    '#title' => t('Your Patron Type'),
    '#options' => $patron_types,
    '#required' => TRUE,
  );
  $form['Department'] = array(
    '#type' => 'select',
    '#title' => t('Your Department'),
    '#options' => $departments,
    '#required' => TRUE,
  );
  $form['DeliveryAddress'] = array(
    '#type' => 'textfield',
    '#title' => t('Your Email'),
    '#size' => 50,

    '#required' => TRUE,
  );
  $form['InstitutionName'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of Institution'),
    '#size' => 50,
    '#default_value' => 'University of Prince Edward Island',
    '#required' => TRUE,
  );
  $form['LibraryName'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of Institution'),
    '#size' => 50,
    '#default_value' => 'Robertson Library',
    '#required' => TRUE,
  );
  $form['notes'] = array(
    '#type' => 'textarea',
    '#title' => t('Notes'),
    '#size' => 50,
    '#required' => FALSE,
  );
  $form['certify'] = array(
    '#type' => 'checkbox',
    '#description' => 'I hereby certify that this item is being sought for a fair dealing purpose and any copying will be in accordance with the Copyright Act. ',
    '#size' => 50,
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );

  return $form;

}


/**
 * The bibliographic portion of the ILL form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_request_form($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $form['#prefix'] = '<div id="upei-roblib-ill-request-form">';
  $form['#suffix'] = '</div>';
  $genre = empty(upei_roblib_ill_get_request_variable('genre')) ? 'article' : upei_roblib_ill_get_request_variable('genre');
//$values = isset($form_state['multistep_values']['upei_roblib_ill_request']) ? orm_state['multistep_values']['ra$fte_the_room'] : array();
  $form['Genre'] = array(
    '#type' => 'select',
    '#title' => t('Genre'),
    '#default_value' => $genre,
    '#options' => array(
      'article' => t('Journal Article'),
      'book' => t('Book'),
      'chapter' => t('Book Chapter'),
    ),
    '#required' => TRUE,
  );
  $form['doi'] = array(
    '#type' => 'textfield',
    '#title' => t('DOI'),
    '#default_value' => upei_roblib_ill_get_request_variable('doi'),
    '#description' => 'DOI, if you provide a DOI we will attempt to lookup the record for you',
    '#size' => 50,
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
    '#required' => FALSE,
  );
  $form['doi_button'] = array(
    '#type' => 'submit',
    '#value' => 'lookup DOI',
    '#ajax' => array(
      'callback' => 'upei_roblib_ill_doi_callback',
      'method' => 'replace',
      'wrapper' => 'upei-roblib-ill-request-form',
      'effect' => 'fade',
    ),
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
    '#required' => FALSE,
  );
  $form['Title'] = array(
    '#type' => 'textarea',
    '#rows' => 2,
    '#title' => t('Source Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('title'),
    '#description' => 'Journal or Book Title',
    '#size' => 50,
    '#required' => FALSE,
  );
  $form['ArticleTitle'] = array(
    '#type' => 'textarea',
    '#rows' => 2,
    '#title' => t('Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('atitle'),
    '#description' => 'Article or Chapter Title',
    '#size' => 50,
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
    '#required' => FALSE,
  );

  $form['ISSN'] = array(
    '#type' => 'textfield',
    '#title' => t('ISSN'),
    '#default_value' => upei_roblib_ill_get_request_variable('issn'),
    '#size' => 50,
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
    '#required' => FALSE,
  );
  $form['ArticleAuthor'] = array(
    '#type' => 'textfield',
    '#title' => t('Author'),
    '#default_value' => upei_roblib_ill_get_request_variable('author'),
    '#size' => 50,
    '#required' => FALSE,
    '#description' => 'Article or Chapter Author',
    '#states' => array(
      'invisible' => array(
        ':input[name="Genre"]' => array('value' => 'book'),
      ),
    ),
  );
  $form['Author'] = array(
    '#type' => 'textfield',
    '#title' => t('Book Author'),
    '#default_value' => upei_roblib_ill_get_request_variable('author'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => array(
      'invisible' => array(
        ':input[name="Genre"]' => array('value' => t('article')),
      ),
    ),
  );
  $form['ISBN'] = array(
    '#type' => 'textfield',
    '#title' => t('ISBN'),
    '#default_value' => upei_roblib_ill_get_request_variable('isbn'),
    '#size' => 50,
    '#states' => array(
      'invisible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
    '#required' => FALSE,
  );
  $form['SubTitle'] = array(
    '#type' => 'hidden',
    '#title' => t('Sub Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('ubtitle'),
    '#size' => 50,
    '#required' => FALSE,
  );
  $form['SeriesTitle'] = array(
    '#type' => 'hidden',
    '#title' => t('Series Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('stitle'),
    '#size' => 50,
    '#required' => FALSE,
  );

  $form['Volume'] = array(
    '#type' => 'textfield',
    '#title' => t('Volume'),
    '#default_value' => upei_roblib_ill_get_request_variable('volume'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
  );
  $form['Issue'] = array(
    '#type' => 'textfield',
    '#title' => t('Issue'),
    '#default_value' => upei_roblib_ill_get_request_variable('issue'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => array(
      'visible' => array(
        ':input[name="Genre"]' => array('value' => 'article'),
      ),
    ),
  );
  $form['ImageOrPageNumber'] = array(
    '#type' => 'textfield',
    '#title' => t('Start Page'),
    '#default_value' => upei_roblib_ill_get_request_variable('spage'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => array(
      'invisible' => array(
        ':input[name="Genre"]' => array('value' => 'book'),
      ),
    ),
  );
  $form['PagesRequested'] = array(
    '#type' => 'textfield',
    '#title' => t('End Page'),
    '#default_value' => upei_roblib_ill_get_request_variable('epage'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => array(
      'invisible' => array(
        ':input[name="Genre"]' => array('value' => 'book'),
      ),
    ),
  );
  $form['Date'] = array(
    '#type' => 'textfield',
    '#title' => t('Date'),
    '#default_value' => upei_roblib_ill_get_request_variable('date'),
    '#size' => 50,
    '#required' => FALSE,
  );
  $form['next'] = array(
    '#type' => 'submit',
    '#value' => t('Next')
  );
  return $form;

}
