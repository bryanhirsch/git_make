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

  private function __construct($prefix,
                               $repo = '',
                               $commit_id = '',
                               $message = '',
                               $squash = TRUE,
                               $branch = '',
                               $root = '') {

    $this->setPrefix($prefix);
    $this->setRepo($repo);
    $this->setCommit_id($commit_id);
    $this->setMessage($message);
    $this->setSquash($squash);
    $this->setBranch($branch);
    $this->setRoot($root);

  }

  /**
   * gets ================================================================================================= 
   */
  private function getPrefix() { if ($this->prefix) { return $this->prefix; } else { return ''; }}
  private function getRepo() { if ($this->repo) { return $this->repo; } else { return ''; }}
  private function getCommit_id() { if ($this->commit_id) { return $this->commit_id; } else { return ''; }}
  private function getMessage() { if ($this->message) { return $this->message; } else { return ''; }}
  private function getSquash() { if ($this->squash) { return $this->squash; } else { return ''; }}
  private function getBranch() { if ($this->branch) { return $this->branch; } else { return ''; }}
  private function getRoot() { if ($this->root) { return $this->root; } else { return ''; }}

  /**
   * sets ================================================================================================= 
   */
  private function setPrefix($prefix) { $this->prefix = $prefix; }
  private function setRepo($repo) { $this->repo = $repo; }
  private function setCommit_id($commit_id) { $this->commit_id = $commit_id; }
  private function setMessage($message) { $this->message = $message; }
  private function setSquash($squash) { $this->squash = $squash; }
  private function setBranch($branch) { $this->branch = $branch; }
  private function setRoot($root) { $this->root = $root; }

  /**
   * Execute git subtree add.
   *
   * @param $test
   *  Boolean. 
   */
  private function add() {
    $message = ($this->getMessage()) ? "--message=$message" : '';
    $squash = ($this->getSquash()) ? "--squash" : '';
    $prefix = "--prefix=" . $this->getPrefix;
    $repo = $this->getRepo();
    $branch = $this->getBranch();

    $statement = "git subtree add $message $squash $prefix $repo $branch";
    return $statement;
  }

  /**
   * Execute git subtree pull.
   */
  private function pull() {
    
  }

  /**
   * Execute git subtree merge.
   */
  private function merge() {}

  // function push() {}
  // function split() {}


}
