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

  public function __construct($prefix,
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
   * gets ========================================================================
   */
  public function getPrefix() { if ($this->prefix) { return $this->prefix; } else { return ''; }}
  public function getRepo() { if ($this->repo) { return $this->repo; } else { return ''; }}
  public function getCommit_id() { if ($this->commit_id) { return $this->commit_id; } else { return ''; }}
  public function getMessage() { if ($this->message) { return $this->message; } else { return ''; }}
  public function getSquash() { if ($this->squash) { return $this->squash; } else { return ''; }}
  public function getBranch() { if ($this->branch) { return $this->branch; } else { return ''; }}
  public function getRoot() { if ($this->root) { return $this->root; } else { return ''; }}

  /**
   * sets ========================================================================
   */
  public function setPrefix($prefix) { $this->prefix = $prefix; }
  public function setRepo($repo) { $this->repo = $repo; }
  public function setCommit_id($commit_id) { $this->commit_id = $commit_id; }
  public function setMessage($message) { $this->message = $message; }
  public function setSquash($squash) { $this->squash = $squash; }
  public function setBranch($branch) { $this->branch = $branch; }
  public function setRoot($root) { $this->root = $root; }

  /**
   * @return string
   *  Custom commit message.
   */
  public function message() {
   return ($this->getMessage()) ? sprintf('--message="%s"', $this->message) : '';
  }

  /**
   * @return string
   *  Squash flag.
   */
  public function squash() {
    return ($this->getSquash()) ? '--squash' : '';  
  }

  /**
   * @return string
   *  Prefix flag.
   */
  public function prefix() {
    return sprintf('--prefix="%s"', $this->getPrefix());
  }

  /**
   * git subtree add
   */
  public function add() {
    $message = $this->message();
    $squash = $this->squash();
    $prefix = $this->prefix();
    $repo = $this->getRepo();
    $branch = $this->getBranch();

    $statement = "git subtree add $message $squash $prefix $repo $branch";
    return $statement;
  }

  /**
   * Execute git subtree pull.
   */
  public function pull() {
    
  }

  /**
   * Execute git subtree merge.
   */
  public function merge() {}

  // function push() {}
  // function split() {}


}
