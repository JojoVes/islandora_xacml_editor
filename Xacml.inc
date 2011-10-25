<?php

/**
 * @file
 * This file defines a set of object for manipulating XACML. Other files in the
 * XACML module provide a lower level access to creating XCAML these objects
 * work together to provide a nice high level view of a standard islandora
 * XACML object.
 */

define('MANAGEMENT_RULE', 'deny-management-functions');
define('DATASTREAM_RULE', 'deny-dsid-mime');
define('VIEWING_RULE', 'deny-access-functions');
define('PERMIT_RULE', 'allow-everything-else');

/**
 * This abstract class represents a general XACML Rule. The XACML object contains 4 standard
 * XACML rules, which are all extended from this base class.
 */
abstract class XacmlRule {

  /**
   * Private internal representation of the XACML rule.
   *
   * @var array() containing rules that can be parsed by XacmlWriter and XacmlParser
   */
  protected $rule;

  /**
   * This points to the Xacml object that this rule is instantiated inside of, so that
   * references to other rules can be made.
   *
   * @var Xacml
   */
  protected $xacml;

  /**
   * Initialized a rule datastructure for XacmlWriter.
   *
   * @param $id
   *   Takes the ID for the new rule as a string.
   * @param $effect
   *   The effect of the rule (Permit or Deny)
   * @return array
   *   A structure that is parsable by XacmlWriter.
   */
  protected function initializeRule($id, $effect) {
    $rule = array();

    $rule['ruleid'] = $id;
    $rule['effect'] = $effect;

    $rule['dsids'] = array();
    $rule['mimes'] = array();
    $rule['methods'] = array();
    $rule['users'] = array();
    $rule['roles'] = array();

    return $rule;
  }

  /**
   * Helper function. Allows strings or arrays of strings to be passed in.
   *
   * @param $type
   *   Array key to modify in internal $rules datastructure.
   * @param $data
   *   Data to be added.
   */
  protected function setValue($type, $data) {
    if(is_array($data)) {
      $this->rule[$type] = array_merge($this->rule[$type], array_values($data));
    }
    else {
      $this->rule[$type][] = $data;
    }
  }

  /**
   * Helper function. Internal arrays may have repeated values. Fixes this before returning.
   *
   * @param $type
   *   Array key in internal datastructure to return
   * @return
   *   Array requested.
   */
  protected function getValues($type) {
    return array_unique($this->rule[$type]);
  }

  /**
   * Uses the array_diff functionality to remove data from internal rule representation.
   *
   * @todo This could all be made more efficient.
   * @param $type
   *   Array key to work on
   * @param $data
   *   Data to be removed.
   */
  protected function removeValues($type, $data) {
    if(!is_array($data)) {
      $data = array($data);
    }

    $this->rule[$type] = array_diff($this->rule[$type],$data);
  }

  /**
   * Constructs new XacmlRule. This generic constructor does not set any
   * methods. It assumes if arg1 is an array that array is an existing
   * xacml rule datastructure. Concrete implementations should call
   * parent::__construct then initialize the datastructure correctly if
   * arg1 is NULL by calling parent::initializeRule() with the proper
   * methods.
   *
   * @param $arg1
   *   array containing pre-exisitng xacml rule or NULL.
   * @param $xacml
   *   reference to the XACML object that this datastructure is part of.
   */
  function __construct($arg1, $xacml) {
    if(is_array($arg1)) {
      $this->rule = $arg1;
      /* remove them now, add them later */
      $this->removeValues('users', 'fedoraAdmin');
      $this->removeValues('roles', 'administrator');
    }

    $this->xacml = $xacml;
  }

  /**
   * Returns true if the rule is populated with data, otherwise returns false.
   *
   * For example a rule can be created that has no users or roles. This rule has no meaning
   * in XACML. We need Users and Roles associated with the rule. This function lets us know
   * if the rule has be populated.
   *
   * @return boolean
   */
  function isPopulated() {
    return $this->getUsers() || $this->getRoles();
  }

  /**
   * Add a user to the XACML rule.
   *
   * @param $user
   *   String or array or strings containing users to add.
   */
  function addUser($user) {
    $this->setValue('users', $user);
  }

  /**
   * Add roles to the XACML rule.
   *
   * @param $role
   *   String or array of string containing roles to add.
   */
  function addRole($role) {
    $this->setValue('roles', $role);
  }

  /**
   * Remove users from XACML Rule.
   *
   * @param $user
   *   String or array of strings with users to remove.
   */
  function removeUser($user) {
    $this->removeValues('users', $user);
  }

  /**
   * Remove roles from XACML rule.
   *
   * @param $role
   *   String or array of string with roles to remove.
   */
  function removeRole($role) {
    $this->removeValues('roles', $role);
  }

  /**
   * Get users associated with this XACML rule.
   *
   * @return
   *   Array containing the users.
   */
  function getUsers() {
    return $this->getValues('users');
  }

  /**
   * Get roles associated with this XACML rule.
   *
   * @return
   *   Array containing the roles.
   */
  function getRoles() {
    return $this->getValues('roles');
  }

  /**
   * Return the $rule datastructure associated with this object. This can be parsed by XacmlWriter.
   * While the above functions only give the users and roles explicitly added to this object, this
   * returns the datastructure containing all users and role. It makes sure that the fedoraAdmin user
   * and administrator role are always added so they we don't lock administrators out of objects.
   *
   * @return
   *   array containing the datastructure.
   */
  function getRuleArray() {
    /* make sure fedora admin can see everything */
    $rule = $this->rule;
    $rule['users'][] = 'fedoraAdmin';
    $rule['roles'][] = 'administrator';
    $rule['users'] = array_unique($rule['users']);
    $rule['roles'] = array_unique($rule['roles']);
    return $rule;
  }
}

/**
 * This is the concrete implementation of XacmlRule for the rule restricting who can manage an object.
 */
class XacmlManagementRule extends XacmlRule {

  /**
   * This calls the parent constructor and then if $arg1 is NULL instantiates the rule as a new
   * blank rule.
   *
   * @param $arg1
   *   Existing Rule datastructure with ID MANAGEMENT_RULE or NULL
   * @param $xacml
   *   Reference to the parent XACML object.
   */
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(MANAGEMENT_RULE, 'Deny');
      $this->rule['methods'] = array(
        'addDatastream',
        'addDisseminator',
        'adminPing',
        'getDisseminatorHistory',
        'getNextPid',
        'ingest',
        'modifyDatastreamByReference',
        'modifyDatastreamByValue',
        'modifyDisseminator',
        'modifyObject',
        'purgeObject',
        'purgeDatastream',
        'purgeDisseminator',
        'setDatastreamState',
        'setDisseminatorState',
        'setDatastreamVersionable',
        'compareDatastreamChecksum',
        'serverShutdown',
        'serverStatus',
        'upload',
        'dsstate',
        'resolveDatastream',
        'reloadPolicies',
      );
    }
  }
}

/**
 * This is the concrete implementation of XacmlRule for the rule restricting who can view an object.
 */
class XacmlViewingRule extends XacmlRule {

  /**
   * This calls the parent constructor and then if $arg1 is NULL instantiates the rule as a new
   * blank rule.
   *
   * @param $arg1
   *   Existing Rule datastructure with ID VIEWING_RULE or NULL
   * @param $xacml
   *   Reference to the parent XACML object.
   */
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(VIEWING_RULE, 'Deny');
      $this->rule['methods'] = array(
        'api-a',
        'getDatastreamHistory',
        'listObjectInResourceIndexResults',
      );
    }
  }

  /**
   * Calls parent::getRuleArray() and then adds the roles and users fromt the managementRule and datastreamRule
   * datastructues if they are populated. This ensures that our xacml object works as expected. Otherwise it would
   * be possible to have people that could manage an object but not view datastreams. An unexpected behavior.
   *
   * @return
   *   $rule datastructure parsable by XacmlWriter.
   */
  function getRuleArray() {
    $rule = parent::getRuleArray();
    if($this->xacml->managementRule->isPopulated()) {
      $rule['users'] = array_unique(array_merge($rule['users'], $this->xacml->managementRule->getUsers()));
      $rule['roles'] = array_unique(array_merge($rule['roles'], $this->xacml->managementRule->getRoles()));
    }
    if($this->xacml->datastreamRule->isPopulated()) {
      $rule['users'] = array_unique(array_merge($rule['users'], $this->xacml->datastreamRule->getUsers()));
      $rule['roles'] = array_unique(array_merge($rule['roles'], $this->xacml->datastreamRule->getRoles()));
    }
    return $rule;
  }
}

/**
 * This is a concrete implementaion of a XacmlRule that allows everything. It needs to be added to the end
 * of every XACML policy to allow anything not explicitly forbidden by the policy. Otherwise XACML defaults to
 * denying access.
 *
 * This is entirely managed by Xacml object so not much needs to be said about it.
 */
class XacmlPermitEverythingRule extends XacmlRule {
  function  __construct($xacml) {
    parent::__construct(NULL, $xacml);
    $this->rule = $this->initializeRule(PERMIT_RULE, 'Permit');
  }

  function getRuleArray() {
    /* make sure fedora admin can see everything */
    return $this->rule;
  }

}

/**
 * A concrete implementation of XacmlRule to restrict who can view certain mimetypes and datastreams.
 */
class XacmlDatastreamRule extends XacmlRule {

  /**
   * Initialized the rule.
   *
   * @param $arg1
   *   NULL or an esiting $rule array with ID DATASTREAM_RULE
   * @param $xacml
   *   Reference to parent Xacml object.
   */
  function  __construct($arg1, $xacml) {
    parent::__construct($arg1, $xacml);
    if($arg1 == NULL){
      $this->rule = $this->initializeRule(DATASTREAM_RULE, 'Deny');
      $this->rule['methods'][] = 'getDatastreamDissemination';
    }
  }

  /**
   * Calls parent::getRuleArray() and then adds the roles and users fromt the managementRule object if they
   * are populated. This ensures that our xacml object works as expected. Otherwise it would
   * be possible to have people that could manage an object but not view datastreams. An unexpected behavior.
   *
   * @return
   *   $rule datastructure parsable by XacmlWriter.
   */
  function getRuleArray() {
    $rule = parent::getRuleArray();
    $rule['dsids'] = $this->getValues('dsids');
    $rule['mimes'] = $this->getValues('mimes');
    if($this->xacml->managementRule->isPopulated()) {
      $rule['users'] = array_unique(array_merge($rule['users'], $this->xacml->managementRule->getUsers()));
      $rule['roles'] = array_unique(array_merge($rule['roles'], $this->xacml->managementRule->getRoles()));
    }

    return $rule;
  }

  /**
   * Add a dsid to the rule.
   *
   * @param $dsid
   *   String or array of strings containing the datastream to add.
   */
  function addDsid($dsid) {
    $this->setValue('dsids', $dsid);
  }

  /**
   * Add a mimetype to the rule.
   *
   * @param $mime
   *   String or array of strings to add to the rule.
   */
  function addMimetype($mime) {
    $this->setValue('mimes', $mime);
  }

  /**
   * Remove mimetypes from the rule.
   *
   * @param $mime
   *   String or array ofs tring to remove from the rule.
   */
  function removeMimetype($mime) {
    $this->removeValues('mimes', $mime);
  }

  /**
   * Remove dsids from the rule.
   *
   * @param $dsid
   *   String or array of strings to remove from the rule.
   */
  function removeDsid($dsid) {
    $this->removeValues('dsids', $dsid);
  }

  /**
   * Mimetypes associated with this rule.
   *
   * @return
   *   array of mimetypes.
   */
  function getMimetypes() {
    return $this->getValues('mimes');
  }

  /**
   * Dsids associated with this rule.
   *
   * @return
   *   array of dsids.
   */
  function getDsids() {
    return $this->getValues('dsids');
  }

  /**
   * Returns true if the rule is populated with data, otherwise returns false.
   *
   * For example a rule can be created that has no users, roles, dsids or mimetypes.
   * This makes sure there is at least on role or user and at least one mimtype or dsid.
   *
   * @return boolean
   */
  function isPopulated() {
    return parent::isPopulated() && ($this->getMimetypes() || $this->getDsids());
  }
}

/**
 * This class is how programmers should interact with Xacml objects. It takes either xacml XAML as a string
 * or no arguements and creates a blank xacml object. The interaction with the rules takes place through
 * member object of this class. For instance to add roles that can manage the object:
 * @code
 *   xacml = new Xacml();
 *   // allow userA to manage the object
 *   xacml->managementRule->addUser('userA');
 *   // allow roleC and roleD to manage the object
 *   xacml->managementRule->addRole(array('roleC', 'roleD'));
 * @endcode
 */
class Xacml {

  /**
   * The $xacml datastructure parsable by XacmlWriter and XacmlParser.
   * @var array()
   */
  protected $xacml;
  /**
   * Rule to allow anything. Users shouldn't need to interact with this.
   * @var XacmlPermitEverythingRule
   */
  protected $permitEverythingRule;

  /**
   * Rule controling who can manage the object with this XACML policy.
   * @var XacmlManagementRule
   */
  public $managementRule;
  /**
   * Rule controlling who can view the specific datastreams and mimetypes that are in this rule.
   * @var XacmlDatastreamRule
   */
  public $datastreamRule;
  /**
   * Rule controlling who can view datastreams in this object.
   * @var XacmlViewingRule
   */
  public $viewingRule;

  /**
   * Initializes the $xacml datastructure that can be parsed with XacmlWriter.
   * @return array
   */
  protected function initializeXacml() {
    // create the rule array
    $xacml = array(
      'RuleCombiningAlgId' => 'urn:oasis:names:tc:xacml:1.0:rule-combining-algorithm:first-applicable',
      'rules'              => array(),
    );
    return $xacml;
  }

  /**
   * The constructor for the XACML object. Initialize new XACML object.
   *
   * @param (optional) $xacml The XACML XML as a string. If this isn't passed
   *   the constructor will instead create a new XACML object that permits
   *   everything.
   * @throws XacmlException if the XML cannot be parsed
   */
  function __construct($xacml = NULL) {
    include_once('XacmlParser.inc');

    $management_rule = NULL;
    $datastream_rule = NULL;
    $viewing_rule = NULL;

    if($xacml != NULL) {
      $this->xacml = XacmlParser::parse($xacml);

      // decide what is enabled
      foreach($this->xacml['rules'] as $rule) {
        if($rule['ruleid'] == MANAGEMENT_RULE)
          $management_rule = $rule;
        elseif ($rule['ruleid'] == DATASTREAM_RULE)
          $datastream_rule = $rule;
        elseif ($rule['ruleid'] == VIEWING_RULE)
          $viewing_rule = $rule;
      }
    }
    else {
      $this->xacml = $this->initializeXacml();
    }

    $this->datastreamRule = new XacmlDatastreamRule($datastream_rule, $this);
    $this->managementRule = new XacmlManagementRule($management_rule, $this);
    $this->viewingRule = new XacmlViewingRule($viewing_rule,$this);
    $this->permitEverythingRule = new XacmlPermitEverythingRule($this);
  }

  /**
   * This function updates the rules array before it is passed to XacmlWriter. It takes into
   * account which rules have been populated.
   */
  private function updateRulesArray() {
    $this->xacml['rules'] = array();

    if($this->datastreamRule->isPopulated())
      $this->xacml['rules'][] = $this->datastreamRule->getRuleArray();
    
    if($this->managementRule->isPopulated())
      $this->xacml['rules'][] = $this->managementRule->getRuleArray();
    
    if($this->viewingRule->isPopulated())
      $this->xacml['rules'][] = $this->viewingRule->getRuleArray();

    $this->xacml['rules'][] = $this->permitEverythingRule->getRuleArray();
  }

  /**
   * Returns the DomDocument that is associated with this Xacml Rule.
   *
   * @return DomDocument
   */
  function getDomDocument() {
    include_once('XacmlWriter.inc');
    $this->updateRulesArray();
    return XacmlWriter::toDOM($this->xacml);
  }

  /**
   * Returns a string containing the XML for this XACML policy.
   *
   * @param boolean $prettyPrint
   *   If set to TRUE the function will return a prettyprinted xacml policy.
   *
   * @return string containing xacml xml
   */
  function getXmlString($prettyPrint=TRUE) {
    include_once('XacmlWriter.inc');
    $this->updateRulesArray();
    return XacmlWriter::toXML($this->xacml, $prettyPrint);
  }
}