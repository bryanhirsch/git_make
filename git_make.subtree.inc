<?php
/**
 * @file git_make.subtree.inc
 * 
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
  public function add($test = FALSE) {
    $message = ($this->message) ? "--message=$message" : '';
    $squash = ($this->squash) ? "--squash" : '';
    $prefix = "--prefix=$this->prefix";
    $repo = $this->repo;
    $branch = $this->branch;

    $statement = "git subtree add $message $squash $prefix $repo $branch";
    return $this->exec($statement, $test);
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


  function exec($command, $test = FALSE) {
    // TODO add cd to top-level first / $this->root;
    if (!$test) {
      return shell_exec($command);
    }
    else {
      return 'TESTING: ' . $command;
    }
  }

}
