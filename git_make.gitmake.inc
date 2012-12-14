<?php
/**
 * @file git_make.gitmake.inc
 *
 * Provides GitMake class.
 */

/**
 * This class builds and returns commands to be executed by git_make.
 */
class GitMake {
  private $contrib_destination;
  private $info;
  private $build_path;
  private $message;
  private $root;

  public function __construct($info, $contrib_destination = 'sites/all/modules') { 
    $this->setInfo($info);
    $this->setContribDestination($contrib_destination);
    $this->setRoot();
  }

  /**
   * gets ========================================================================
   */
  //public function getPrefix() { if ($this->prefix) { return $this->prefix; } else { return ''; }}

  /**
   * sets ========================================================================
   */
  public function setInfo($info) { $this->info = $info; }
  public function setContribDestination($contrib_destination = 'sites/all/modules') { $this->contrib_destination = $contrib_destination; }
  public function setRoot() { $this->root = getcwd(); }

  /**
   * @param string $command
   *  (Optional) git-subtree command being executed.
   *
   * @return string
   *  Custom commit message.
   */
  public function message($command = FALSE) {
    /*
    if ($command) {
      if ($message = $this->getMessage()) {
        $message = sprintf("%s\n\tgit subtree %s", $message, $command);
      }
      else {
        $message = "git subtree $command";
      }
      $this->setMessage($message);
    }

   return ($this->getMessage()) ? sprintf('--message="%s"', $this->message) : '';
   // */
  }

  /**
   * @param array $project
   *
   * @return string $url
   *  URL of git repo.
   */
  static public function getDownloadUrl($project) {
    $download = $project['download'];
    $name = $project['name'];

    // If no download URL specified, assume anonymous clone from git.drupal.org.
    $url = isset($download['url']) ? $download['url'] : "http://git.drupal.org/project/$name.git";

    return $url;
  }

  /**
   * Check to see if repo exists.
   *
   * @param $repo
   *  String. /path/to/local/repo
   *
   * @return
   *  Boolean
   */
  static public function localRepoExists($repo) {
    drush_log(sprintf('Checking for %s', $repo));

    $is_directory = is_dir($repo);
    $is_repo = is_dir("$repo/.git");

    if (!$is_directory && !$is_repo) {
      drush_log(sprintf('Repo does not exist: %s', $repo));
      return FALSE;
    }

    drush_log(sprintf('Repo exists: %s', $repo));

    return TRUE;
  }

  /**
   * Get a local copy of the project's git repo.
   *
   * @param array $project
   *
   * @param boolean $checkout
   *  Checkout branch.
   *
   * @return $repo
   *  String /path/to/repo.
   */
  public function getLocalRepo($project, $checkout = TRUE) {
    $name = $project['name'];
    $url = git_make_download_url($project);

    // Check for local copy of project repo. If it doesn't exist
    // put it here: .drush/repos/$name. If it does exist, try to
    // update it.
    //
    // TODO Add option for user to designate other location.
    // TODO Add option in make file to specify remote(s).
  //drush_print(print_r($_ENV));
    $home = $_ENV['HOME'];
    $repos = "$home/.drush-repos";
    if (!is_dir($repos)) {
      $mode = 0777;
      $recursive = TRUE;
      mkdir($repos, $mode, $recursive);
    }
    $repo = sprintf("%s/%s", $repos, $name);
    $use_local = drush_get_option('local');
    $repo_exists = git_make_local_repo_exists($repo);

    if ($repo_exists && $use_local) {
      // Use local checkout of repo as-is. Do nothing here.
      drush_log(sprintf("Using local repo: %s\n", $repo));
    }
    else if ($repo_exists) {
      // Repo exists. Update existing local repo.
      drush_log(sprintf("Checking %s for updates to %s\n", $url, $repo));
      $command = sprintf('cd %s; git pull', $repo);
      _git_make_exec($command);
    }
    else {
      // Repo doesn't exist. Create repo.
      drush_log(sprintf('Cloning %s to %s...', $url, $repo));
      $command = sprintf('cd %s; git clone %s %s', $repos, $url, $name);
      _git_make_exec($command);
      // TODO Add some error handling. E.g. if clone fails.
    }

    // Checkout branch that's going to be pulled. Since we're not cloning from
    // bare repos, we need a local copy of whatever branch we're going to use.
    // This branch doesn't actually have to be checked out while we're pulling.
    // This just seems like the easiest way to handle all scenarios.
    // While we're here do some garbage collection for good measure.
    // @todo Add an option to skip gc;
    if ($checkout) {
      $branch = git_make_get_branch($project);
      $command = sprintf('cd %s; git checkout %s; git gc;', $repo, $branch);
      _git_make_exec($command);
    }

    return $repo;
  }

  /**
   * Get commit IDs for project version.
   *
   * @param $core
   *  String. Version of Drupal. e.g. 7.x
   *
   * @param $project
   *  Array.
   *
   * @param $repo
   *  String. /path/to/local/repo
   *
   * @return $id
   *  String. Git commit ID.
   */
   static public function getId($core, $project, $repo = FALSE) {
      $id = '';

      // Get path to local repo if we don't already know it.
      if (!$repo) {
        $repo = git_make_get_local_repo($project);
      }

      // If make file designates a git commit ID, that's all we need.
      $revision = (isset($project['download']['revision'])) ? $project['download']['revision'] : FALSE;
      if ($type == 'git' && $revision) {
        $id = $revision;
      }
      // Get the commit ID based on the tag specified in the make file.
      else if ($tag = git_make_get_tag($project)) {
        $id = git_make_get_id_from_tag($repo, $tag);
      }
      // Get the commit ID based on the release version specified in
      // the make file.
      else if ($version = git_make_get_version($project)) {
        // If the project is following Drupal tagging conventions we should
        // be able to figure out the tag we're looking for.
        $tag = ($project['name'] == 'drupal') ? $version : sprintf('%s-%s', $core, $version);
        $id = git_make_get_id_from_tag($repo, $tag);
      }
      // No version is specified in the make file, but a branch is specified,
      // use that.
      else if ($branch = git_make_get_branch($project)) {
        // Find out the commit ID at the tip of $branch.
        $id = git_make_get_id_from_branch($repo, $branch);
      }
      /*
      // Try to get $id based on recommended version on drupal.org.
      else if ($version = git_make_get_recommended_version()) {
        // TODO
      }
      /*
      else {
       // TODO
      }
      // */

      return $id;
    }

    /**
     * @param $repo
     *  String. /path/to/local/repo
     *
     * @param $tag
     *  String. Git tag.
     *
     * @return $id
     *  String. Git commit ID (SHA1) corresponding to tag.
     */
    static public function getIdFromTag($repo, $tag) {
      $cd = "cd $repo";
      $get_id = "git rev-parse $tag";
      $command = "$cd; $get_id;";
      // TRUE tells drush to run this even when user passes 'print-only' option.
      $id = _git_make_exec($command, TRUE);
      return trim($id);
    }

    /**
     * @param $repo
     *  String. /path/to/local/repo
     *
     * @param $tag
     *  String. Git tag.
     *
     * @return $id
     *  String. Git commit ID (SHA1) corresponding to tag.
     */
    static public function getIdFromBranch($repo, $branch) {
      $cd = "cd $repo";
      $checkout_branch = "git checkout $branch";
      $get_id = "git rev-parse HEAD";
      $command = "$cd; $checkout_branch; $get_id;" ;
      // TRUE tells drush to run this even when user passes 'print-only' option.
      $id = _git_make_exec($command, TRUE);
      return trim($id);
    }

    /**
     * Build standard drupal project path.
     *
     * Note: You HAVE to build from the top-level directory with git-make.
     * Assuming Drupal docroot is always a child or subtree of some parent repo.
     *
     * @return
     *   String. path/to/project
     */
    static public function getProjectPath($type, $project) {
      // E.g. $prefix = docroot
      $build_path = $project['build_path'];
      $prefix = $build_path;

      // Build contrib path.
      if ($type == 'contrib') {
        if ($project['type'] != 'profile') {
          // E.g. $prefix = docroot/sites/all
          $prefix .= '/' . $project['contrib_destination'];
        }
        // E.g. $prefix = docroot/sites/all/modules
        // Or $prefix = docroot/profiles
        //   TODO Decide how to handle 'libraries' type here...
        $prefix .= '/' . $project['type'] . 's';
        if ($subdir = $project['subdir']) {
          // E.g. $prefix = docroot/sites/all/modules/contrib
          $prefix .= '/' . $subdir;
        }

        if ($directory_name = $project['directory_name']) {
          // E.g. $prefix = docroot/sites/all/modules/contrib/directory_name
          $prefix .= '/' . $directory_name;
        }
        else {
          $prefix .= '/' . $project['name'];
        }
      }

      return $prefix;
    }

    /**
     * Build prefix path to pass to git subtree.
     *
     * This will be used as the --prefix option like this:
     *   git subtree add --prefix=project/project-name <repo> <branch>
     *
     * Note: You HAVE to build from the top-level directory with git-make.
     * Assuming Drupal docroot is always a subtree of some parent repo.
     *
     * @param string $name
     *
     * @return
     *   String. git subtree --prefix value.
     */
    static public function getPrefix($name) {
      return sprintf('projects/%s', $name);
    }

    /**
     * @param $project
     *  Array.
     *
     * @return
     *  String. Git branch.
     */
    static public function getBranch($project) {
      $core = $project['core'];

      if ($project['name'] == 'drupal') {
        $branch = $core;
      }
      else if (!empty($project['download']['branch'])) {
        $branch = $project['download']['branch'];
      }
      else if (!empty($project['version'])) {
        // Turn 1.15 into 1.x
        $project_parts = explode('.', $project['version']);
        $project_version = $project_parts[0] . '.x';
        // Branch!...
        $branch = sprintf('%s-%s', $core, $project_version);
      }
      else {
        drush_log("I couldn't figure out what branch to use, so I'm using master. "
                . "Hope this works.");
        $branch = 'master';
      }

      return $branch;
    }

    /**
     * @param $project
     *  Array.
     *
     * @return string
     *   Project version. E.g. 5.1 for Zen 7.x-5.1.
     *
     * @see git_make_get_tag
     */
    static public function getVersion($project) {
      return (!empty($project['version'])) ? $project['version'] : FALSE;
    }

    /**
     * Get git tag for project.
     *
     * @param $project
     *  Array.
     *
     * @return $tag
     *  String. Git tag or FALSE.
     */
    static public function getTag($project) {
      $tag = FALSE;

      // If tag is explicitly named in the make file, that's all we need.
      if (isset($project['download']['tag'])) {
        $tag = $project['download']['tag'];
      }
      // If a version like 5.1 is given for a project like Zen 7.x-5.1, build the tag.
      else if ($version = git_make_get_version($project)) {
        $tag = ($project['name'] == 'drupal') ? $version : sprintf('%s-%s', $project['core'], $version);
      }

      return $tag;
    }

    /**
     * @param $type
     *  String. core, contrib.
     *
     * @param $info
     *  Array. Parsed make file.
     *
     * @param $project
     *  Array. The project for which we're getting a subtree.
     */
    // TODO get $info from $this
    // TODO get $project from $this ?
    // TODO get $type from $this ?
    public function checkoutIndex($type, $info, $project) {
      $commands = array();

      // Get path to local copy of the project's git repo. Clone or update if needed.
      $repo = git_make_get_local_repo($project, FALSE);

      // Get tag or commit ID for project version.
      if (!$tag = git_make_get_tag($project)) {
        $commit_id = git_make_get_id($info['core'], $project, $repo);
      }
      $revision = ($tag) ? $tag : $commit_id;

      // Set commit message.
      $timestamp = time();
      $message = sprintf('drush git-make updated %s to %s, %s',
                           $project['name'],
                           $revision,
                           date('Y-m-d, H:i', $timestamp));

      // Top-level directory in git repo.
      $root = getcwd();

      // Project prefix.
      $prefix = git_make_get_prefix($project['name']);

      // If the project already exists, remove it.
      if (is_dir($prefix)) {
        $commands[] = sprintf('git rm -r %s/%s', $root, $prefix);
      }

      // Checkout the verision of the project specified in the make file.
      $commands[] = sprintf('cd %s; git checkout %s; git checkout-index -a -f --prefix=%s/%s/ ;',
                                $repo,
                                $revision,
                                $root,
                                $prefix
      );

      $commands[] = git_make_append_project_info($revision, $project['name'], $root, $prefix);
      $commands[] = sprintf('echo "%s" >> %s/%s/%s.info', $text, $root, $prefix, $project['name']);

      // Stage changes.
      $commands[] = sprintf('git add %s/%s', $root, $prefix);

      // Commit.
      $commands[] = sprintf('git commit -m "%s"', $message);

      // Execute.
      foreach ($commands as $command) {
        _git_make_exec($command);
      }

      drush_log($message);
    }

    /**
     * @param array $project
     *
     * @return boolean
     */
    public function downloadTypeIsSubtree($project) {
      if (!empty($project['download']) && $project['download']['type'] == 'git-subtree') {
        return TRUE;
      }
      else {
        return FALSE;
      }
    }

    /**
     * @return string
     *  Statement to append project info to project info file.
     */
    static public function appendProjectInfo($revision, $name, $root, $prefix) {
      // Add version to info file.
      $timestamp = time();
      $text = sprintf("\n; Information added by git_make on %s\n"
                    . "version = %s\n"
                    . "project = %s\n"
                    . "datestamp =  %s\n",
                    date('Y-m-d', $timestamp),
                    $revision,
                    $name,
                    $timestamp);

      $command = sprintf('echo "%s" >> %s/%s/%s.info', $text, $root, $prefix, $name);

      return $command;
    }

    /**
     * @param string $path
     *   Drupal's expected path/to/project
     *
     * @param string $name
     *  Project name
     *
     * @param boolean $is_core
     *
     * @return string $command
     *  To be executed via shell_exec()
     */
    public function addSymlink($path, $name, $is_core = FALSE) {
      $command = '';

      // @todo Make name of docroot customizeable.
      if ($is_core && _git_make_add_docroot('docroot')) {
        $command .= _git_make_add_core_symlinks();
        $command .= _git_make_remove_core_symlinks();
        $command .= _git_make_add_core_directories($path, $name);
      }
      else {
        $command .= _git_make_add_symlink($path, $name);
      }

      return $command;
    }

    /**
     * @param string $path
     *   Drupal's expected path/to/project
     *
     * @param string $name
     *  Project name
     *
     * @param boolean $is_core
     *
     * @return string $command
     *  To be executed via shell_exec()
     */
    static public function _addSymlink($path, $name) {
      $command = '';

      // Change directories to where the symlink should be placed.
      $end = strrpos($path, '/');
      $go_here = substr($path, 0, $end);
      // Make sure "here" exists.
      if (!is_dir($go_here)) {
        $command .= sprintf('mkdir -p %s; ', $go_here);
      }
      $command .= "cd $go_here; ";

      // Get the relative path to the project.
      $parts = explode('/', $go_here);
      $relative_path = '';
      foreach ($parts as $part) {
        $relative_path .= '../';
      }
      // Get prefix (path to the project from top-level directory).
      $prefix = git_make_get_prefix($name);
      $relative_path .= $prefix;

      $command .= sprintf('ln -s %s %s;', $relative_path, $name);

      return $command;
    }

    /**
     * @param string $docroot
     *   Name of root Drupal directory.
     *
     * @return boolean
     *  Success / Failure.
     */
    static public function _addDocroot($docroot = 'docroot') {
      if (!is_dir($docroot)) {
        $result = mkdir('docroot', 0755);
        // @todo Add error handling if mkdir fails.
      }
      else {
        return TRUE;
      }

      return $result;
    }

    /**
     * Add symlinks to files and directories inside Drupal root.
     *
     * @return string $command
     *  To be executed via shell_exec()
     */
    static public function _addCoreSymlinks() {
      // Default: 'cd docroot; ln -s ../project/drupal/* ./ ;';
      $prefix = git_make_get_prefix('drupal');
      $path = sprintf('../%s', $prefix);
      $command = sprintf('cd docroot; ln -s %s/* ./ ; ', $path);

      return $command;
    }

    /**
     * Remove symlinks to drupal/sites and drupal/profiles.
     *
     * @return string $command
     *  To be executed via shell_exec()
     */
    static public function _removeCoreSymlinks() {
      $command = '';

      // cd into docroot.
      $command .= 'cd docroot; ';

      // Remove symlink to ../projects/drupal/sites
      $command .= 'rm sites; ';

      // TODO Only remove symlink to profiles if a profile project is detected in .make file.
      // Remove symlink to ../projects/drupal/profiles.
      $command .= 'rm profiles; ';

      return $command;
    }

    /**
     * @param boolean $is_core
     *
     * @return string $command
     *  Statement to be executed to commit symlinks.
     */
    public function commitSymlinks($is_core = FALSE) {
      $command = 'cd docroot; git add . ; ';

      if ($is_core) {
        $command .= 'git commit -m "Add symlinks to Drupal core."; ';
      }
      else {
        $command .= 'git commit -m "Add symlinks to projects."; ';
      }

      return $command;
    }
    

    /**
     * Add docroot/sites/README and docroot/profiles/README.
     *
     * @return string $command
     *  To be executed via shell_exec()
     */
    static public function _addCoreDirectories() {
      $command = '';

      // Create sites/README.txt;
      $command .= 'mkdir sites; echo "@todo see _git_make_remove_core_symlinks()" >> sites/README.txt ; ';
      $command .= 'git add sites/README.txt ; git commit -m "Add docroot/sites/README.txt by git-make."; ';

      // TODO Only do this if there's a profile in the make file (that is, if
      //   the symlink to ../projects/drupal/profiles was removed).
      // Create profiles/README.txt;
      $command .= 'mkdir profiles; echo "@todo see _git_make_remove_core_symlinks()" >> profiles/README.txt ; ';
      $command .= 'git add sites/README.txt ; git commit -m "Add docroot/profiles/README.txt by git-make."; ';

      return $command;
    }

}
