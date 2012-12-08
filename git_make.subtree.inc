<?php
/**
 * @file git_make.subtree.inc
 */

/**
 * This class builds and returns git subtree commands to be executed by git_make.
 */
class GitSubtree {
  public $prefix;
  public $repo;
  public $commit_id;
  public $message;
  public $squash;
  public $branch;
  public $root;

  public function __construct($prefix, $commit = FALSE, $message = FALSE, $squash = TRUE, $branch = '') {
    $this->prefix = $prefix;
    $this->commit = $commit;
    $this->message = $message;
    $this->squash = $squash;
  }

  /**
   * Execute git subtree add.
   *
   * @param $test
   *  Boolean. 
   */
  public function add() {
    $message = ($this->message) ? "--message=$message" : '';
    $squash = ($this->squash) ? "--squash" : '';
    $prefix = "--prefix=$this->prefix";
    $repo = $this->repo;
    $branch = $this->branch;

    $statement = "git subtree add $message $squash $prefix $repo $branch";
    return $statement;
  }

  /**
   * Execute git subtree pull.
   */
  function pull() {}

  /**
   * Execute git subtree merge.
   */
  function merge() {}

  // function push() {}
  // function split() {}


}
