<?php
/**
 * @file git_make.subtree.inc
 */

/**
 * This class builds and returns git subtree commands to be executed by git_make.
 */
class GitSubtree {
  private $prefix;
  private $repo;
  private $commit_id;
  private $message;
  private $squash;
  private $branch;
  private $root;

  private function __construct($prefix, $commit = FALSE, $message = FALSE, $squash = TRUE, $branch = '') {
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
  private function add() {
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
  private function pull() {}

  /**
   * Execute git subtree merge.
   */
  private function merge() {}

  // function push() {}
  // function split() {}


}
