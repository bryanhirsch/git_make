<?php
/**
 * @file
 * Provide Drush integration for release building and dependency building.
 */
require_once('git_make.subtree.inc');
require_once('git_make.gitmake.inc');

/**
 * Implements hook_drush_help().
 */
function git_make_drush_help($section) {
  switch ($section) {
    case 'drush:git-make':
      return dt('TODO add git-make help stuff here.');
  }
}

/**
 * Implements hook_drush_command().
 */
function git_make_drush_command() {
  $items = array();

  $items['git-make'] = array(
    'description' => '',
    'callback' => 'drush_git_make',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap.
    'arguments' => array(
      'path/to/something-build.make' => 'Build file.',
      'path/to/destination' => '(optional) Build site here.',
    ),
    'options' => array(
      'local' => 'Use local ("cached") copy of repos only.',
      'print-only' => 'Print commands, do not execute them.',
    ),
    'examples' => array(
      'drush git-make build-petition.make docroot' => '',
      'drush git-make build-petition.make docroot --local' => '',
    ),
    'aliases' => array('gm'),
  );

  return $items;
}

/**
 * @param $makefile
 *  String. path/to/makefile
 *
 * @param $build_path
 *  String. The path where the final build will be placed.
 */
function drush_git_make($makefile, $build_path = 'docroot') {
  $info = make_parse_info_file($makefile);

  // TODO Make an option for building site like this, with all
  // prefixes pointing to project/* rather than storing everything
  // inside docroot and getting tangled up with nested projects.
   // projects/drupal
   // projects/x
   // projects/y
   // projects/z
   // docroot/                            -> ../projects/drupal
   // docroot/profiles/x                  -> ../../projects/x
   // docroot/sites/all/modules/contrib/y -> ../../../../../projects/y
   // docroot/sites/default/modules       -> ../../../../sites/default/modules
   // docroot/sites/example.com           -> ../../../sites/example.com
   // sites/default/modules/custom/p      -> ../../../../projects/p
  // TODO store these patches to core in git_make to make this ^^ possible
   // - rm sites/ modules/ profiles/
   // - [mkdir sites/ modules/ profiles/]
   // - apply patch to create sites/default, profiles/standard, modules/README

 // TODO Support merge options in .make:
   // project[example][download][type] = 'git-subtree'
   // project[example][download][squash] = TRUE
   // project[example][download][message] = 'Some message'

  /* TODO @see drush_make
  // Support making just a portion of a make file.
  $include_only = array(
    'projects' => array_filter(drush_get_option_list('projects')),
    'libraries' => array_filter(drush_get_option_list('libraries')),
  );
  $info = make_prune_info_file($info, $include_only);
  // */

  if ($info === FALSE || ($info = make_validate_info_file($info)) === FALSE) {
    return FALSE;
  }

  //if (make_projects(FALSE, drush_get_option('contrib-destination', 'sites/all'), $info, $build_path)) {
  // TODO ^^ This is what the call looks like in drush_make(). Update to do this.
  if (git_make_make_projects(FALSE, drush_get_option('contrib-destination', 'sites/all'), $info, $build_path)) {
    //make_libraries(drush_get_option('contrib-destination', 'sites/all'), $info, $build_path);
    git_make_make_libraries(NULL, $info, $build_path);

    /*
    if (drush_get_option('prepare-install')) {
      make_prepare_install($build_path);
    }
    // */
  }

  return $info;
}

/**
 * Call this instead of running Drush core's make_projects().
 *
 * Use same signature as Drush make_projects:
 * @param $recursion              TODO hard-coded FALSE. Add support for recursion.
 *                                     check for .make file. Then kick of new build"
                                       inside this project.
 * @param $contrib_destination    TODO test different contrib destinations.
 * @param $info
 * @param $build_path
 *
 * @todo Submit patch to Drush make to break the big long make_projects() function
 * into smaller parts that can be reused here.
 */
function git_make_make_projects($recursion = FALSE, $contrib_destination, $info, $build_path = 'docroot') {

  drush_include_engine('release_info', 'updatexml');

  // TODO
  // Check for .git and docroot. Must run from top-level directory.

  // TODO parameterize "docroot".

  $projects = array();

  $translations = array();
  if (isset($info['translations'])) {
    $translations = $info['translations'];
  }

  foreach ($info['projects'] as $key => $project) {
    /*
    // TODO Remove this? This should be unnecesary for Git checkouts since commit IDs are SHA1s. Leave code here for now, in case anything blows up without it. Remove later. Maybe we'll still need this for patches?
    $md5 = '';
    if (isset($project['md5'])) {
      $md5 = $project['md5'];
    }
    // */

    // Merge the known data onto the project info.
    $project += array(
      'name'                => $key,
      'core'                => $info['core'],
      'translations'        => $translations,
      'build_path'          => $build_path,
      'contrib_destination' => $contrib_destination,
      'version'             => '',
      'location'            => drush_get_option('make-update-default-url', RELEASE_INFO_DEFAULT_URL),
      'subdir'              => '',
      'directory_name'      => '',
    );

    if (empty($project['download']['type'])) {
      // No download type is specified in make file. The project lives
      // on drupal.org in a Git repo.
      $project['download']['type'] = 'git';
    }
    else if ($project['download']['type'] != 'git' && $project['download']['type'] != 'git-subtree') {
      // If download is specifiec as something other than git, this isn't
      // going to work.
      $type = $project['download']['type'];
      $name = $project['name'];
      $message = "Sorry. Your make file specifies $type as the download "
               . "type for the project: $name. You need to use a Git "
               . "repo to use git-make.";
      drush_print($message);
    }

    // If the 'revision' parameter is passed at the top level, this is
    // short-hand for download revision.
    if (isset($project['revision']) && !isset($project['download'])) {
      $project['download']['revision'] = $project['revision'];
    }

    if (!isset($project['l10n_url']) && ($project['location'] == RELEASE_INFO_DEFAULT_URL)) {
      $project['l10n_url'] = MAKE_DEFAULT_L10N_SERVER;
    }

    // For convenience: define $request to be compatible with release_info
    // engine.
    // TODO: refactor to enforce 'make' to internally work with release_info
    // keys.
    $request = make_prepare_request($project);

    if ($project['location'] != RELEASE_INFO_DEFAULT_URL && !isset($project['type'])) {
      // Set the cache option based on our '--no-cache' option.
      $cache_before = drush_get_option('cache');
      if (!drush_get_option('no-cache', FALSE)) {
        drush_set_option('cache', TRUE);
      }
      $project_type = release_info_check_project($request, 'core');
      // Restore the previous '--cache' option value.
      drush_set_option('cache', $cache_before);
      $project['download_type'] = ($project_type ? 'core' : 'contrib');
    }
    elseif (!empty($project['type'])) {
      $project['download_type'] = ($project['type'] == 'core' ? 'core' : 'contrib');
    }
    else {
      $project['download_type'] = ($project['name'] == 'drupal' ? 'core' : 'contrib');
    }
    $projects[$project['download_type']][$project['name']] = $project;
  }

  $cores = !empty($projects['core']) ? count($projects['core']) : 0;

  if (drush_get_option('no-core')) {
    unset($projects['core']);
  }
  elseif ($cores == 0 && !$recursion) {
    drush_set_error(dt('No core project specified.'));
    return FALSE;
  }
  elseif ($cores == 1 && $recursion) {
    unset($projects['core']);
  }
  elseif ($cores > 1) {
    drush_set_error(dt('More than one core project specified.'));
    return FALSE;
  }

  foreach ($projects as $type => $type_projects) {
    foreach ($type_projects as $project) {
      if (make_project_needs_release_info($project)) {
        // For convenience: define $request to be compatible with release_info
        // engine.
        // TODO: refactor to enforce 'make' to internally work with release_info
        // keys.
        $request = make_prepare_request($project, $type);
        // Set the cache option based on our '--no-cache' option.
        $cache_before = drush_get_option('cache');
        if (!drush_get_option('no-cache', FALSE)) {
          drush_set_option('cache', TRUE);
        }
        $release = release_info_fetch($request);
        // Restore the previous '--cache' option value.
        drush_set_option('cache', $cache_before);
        // Translate release_info key for project_type to drush make.
        $project['type'] = $request['project_type'];


      }

      /*
      if (!empty($md5)) {
        $project['download']['md5'] = $md5;
      }
      if ($ignore_checksums) {
        unset($project['download']['md5']);
      }
      // @todo Remove ^^? Do we need this for patches?
      //  */

      $projects[($project['type'] == 'core' ? 'core' : 'contrib')][$project['name']] = $project;
    }
  }

  // "Core is built in place, rather than using make-process."
  if (isset($projects['core'])) {
    // If download type is 'git-subtree', get the subtree.
    if (git_make_download_type_is_subtree($projects['core']['drupal'])) {
      git_make_get_subtree('core', $info, $projects['core']['drupal']);
    }
    // If download type is not 'git-subtree', just checkout and commit.
    else {
      git_make_checkout_index('core', $info, $projects['core']['drupal']);
    }

    // TODO
    // If there are patches download and apply patches.

    // Make symlink to project inside docroot.
    $path = git_make_get_project_path('core', $projects['core']['drupal']);
    $command = git_make_add_symlink($path, $projects['core']['drupal']['name'], TRUE);
    _git_make_exec($command);
  }

  // Process all projects.
  if (isset($projects['contrib'])) {
    foreach ($projects['contrib'] as $project) {
      // TODO Consider using drush_backend for something here, like make_projects()?

      // If download type is 'git-subtree', get the subtree.
      if (git_make_download_type_is_subtree($project)) {
        git_make_get_subtree($type, $info, $project);
      }
      // If download type is not 'git-subtree', just checkout and commit.
      else {
        git_make_checkout_index($type, $info, $project);
      }

      // TODO
      // If there are patches download and apply patches.

      // Make symlink to project inside docroot.
      $path = git_make_get_project_path($type, $project);
      $command = git_make_add_symlink($path, $project['name']);
      _git_make_exec($command);
    }
  }

  // Garbage collection.
  $command = 'git gc';
  _git_make_exec($command);

  return TRUE;
  // */
}

/**
 * Call this instead of Drush core's make_libraries().
 *
 * Use same signature as Drush make_libraries:
 * @param $contrib_destination    TODO hard-coded NULL
 * @param $info * @param $build_path */
function git_make_make_libraries($contrib_destination = NULL, $info, $build_path) {
  // TODO
}
// */

/**
 * @param array $project
 *
 * @return string $url
 *  URL of git repo.
 */
// TODO Deprecated. Remove.
function git_make_download_url($project) {
  drush_log('deprecated: Call to git_make_download_url(). Use GitMake::getDownloadUrl().');
  return GitMake::getDownloadUrl($project);
}

// TODO remove this...
function _git_make_stuff_from_make_download_git($name, $download, $download_location) {
  // If no working-copy download URL specified, assume it is the same.
  $download['wc_url'] = isset($download['wc_url']) ? $download['wc_url'] : $download['url'];

  // If not a working copy, and if --no-cache has not been explicitly
  // declared, create a new git reference cache of the remote repository,
  // or update the existing cache to fetch recent changes.
  // @see package_handler_download_project()
  $cache = !$wc && !drush_get_option('no-cache', FALSE);
  if ($cache && ($git_cache = drush_directory_cache('git'))) {
    $project_cache = $git_cache . '/' . $name . '-' . md5($download['url']);
    // Set up a new cache, if it doesn't exist.
    if (!file_exists($project_cache)) {
      $command = 'git clone --mirror';
      if (drush_get_context('DRUSH_VERBOSE')) {
        $command .= ' --verbose --progress';
      }
      $command .= ' %s %s';
      drush_shell_cd_and_exec($git_cache, $command, $download['url'], $project_cache);
    }
    else {
      // Update the --mirror clone.
      drush_shell_cd_and_exec($project_cache, 'git remote update');
    }
    $git_cache = $project_cache;
  }

  // Use working-copy download URL if --working-copy specified.
  $url = $wc ? $download['wc_url'] : $download['url'];

  $tmp_location = drush_tempdir() . '/' . basename($download_location);

  $command = 'git clone %s %s';
  if (drush_get_context('DRUSH_VERBOSE')) {
    $command .= ' --verbose --progress';
  }
  if ($cache) {
    $command .= ' --reference ' . drush_escapeshellarg($git_cache);
  }

  // Before we can checkout anything, we need to clone the repository.
  if (!drush_shell_exec($command, $url, $tmp_location)) {
    make_error('DOWNLOAD_ERROR', dt('Unable to clone @project from @url.', array('@project' => $name, '@url' => $url)));
    return FALSE;
  }

  drush_log(dt('@project cloned from @url.', array('@project' => $name, '@url' => $url)), 'ok');

  // Get the current directory (so we can move back later).
  $cwd = getcwd();
  // Change into the working copy of the cloned repo.
  chdir($tmp_location);

  // We want to use the most specific target possible, so first try a refspec.
  if (!empty($download['refspec'])) {
    if (drush_shell_exec("git fetch %s %s", $url, $download['refspec'])) {
      drush_log(dt("Fetched refspec !refspec.", array('!refspec' => $download['refspec'])), 'ok');

      if (drush_shell_exec("git checkout FETCH_HEAD")) {
        drush_log(dt("Checked out FETCH_HEAD."), 'info');
      }
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to fetch the refspec @refspec from @project.", array('@refspec' => $download['refspec'], '@project' => $name)));
    }
  }

  // If there wasn't a refspec, try a tag.
  elseif (!empty($download['tag'])) {
    // @TODO: change checkout to refs path.
    if (drush_shell_exec("git checkout %s", 'refs/tags/' . $download['tag'])) {
      drush_log(dt("Checked out tag @tag.", array('@tag' => $download['tag'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to check out tag @tag.", array('@tag' => $download['tag'])));
    }
  }

  // If there wasn't a tag, try a specific revision hash.
  elseif (!empty($download['revision'])) {
    if (drush_shell_exec("git checkout %s", $download['revision'])) {
      drush_log(dt("Checked out revision @revision.", array('@revision' => $download['revision'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to checkout revision @revision", array('@revision' => $download['revision'])));
    }
  }

  // If not, see if we at least have a branch.
  elseif (!empty($download['branch'])) {
    if (drush_shell_exec("git checkout %s", $download['branch'])) {
      drush_log(dt("Checked out branch @branch.", array('@branch' => $download['branch'])), 'ok');
    }
    elseif (drush_shell_exec("git checkout -b %s %s", $download['branch'], 'origin/' . $download['branch'])) {
      drush_log(dt('Checked out branch origin/@branch.', array('@branch' => $download['branch'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt('Unable to check out branch @branch.', array('@branch' => $download['branch'])));
    }
  }

  if (!empty($download['submodule'])) {
    $command = 'git submodule update';
    foreach ($download['submodule'] as $option) {
      $command .= ' --%s';
    }
    if (call_user_func_array('drush_shell_exec', array_merge(array($command), $download['submodule']))) {
      drush_log(dt('Initialized registered submodules.'), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt('Unable to initialize submodules.'));
    }
  }

  // Move back to last current directory (first line).
  chdir($cwd);

  // Move the directory into the final resting location.
  drush_copy_dir($tmp_location, $download_location, TRUE);

  // Remove .git/ directory if working-copy flag was not specified.
  if (!$wc && file_exists($download_location . '/.git')) {
    drush_register_file_for_deletion($download_location . '/.git');
  }

  return dirname($tmp_location);
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
// TODO Deprecated. Remove.
function git_make_local_repo_exists($repo) {
  drush_log('deprecated: Call to git_make_local_repo_exists(). Use GitMake::localRepoExists().');
  return GitMake::localRepoExists($repo);
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
function git_make_get_local_repo($project, $checkout = TRUE) {
  drush_log('deprecated: Call to git_make_get_local_repo(). Use GitMake::getLocalRepo().');
  return GitMake::getLocalRepo($project, $checkout);
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
// TODO Deprecated. Remove.
function git_make_get_id($core, $project, $repo = FALSE) {
  drush_log('deprecated: Call to git_make_get_id(). Use GitMake::getId().');
  return GitMake::getId($core, $project, $repo);
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
// TODO Deprecated. Remove.
function git_make_get_id_from_tag($repo, $tag) {
  drush_log('deprecated: Call to git_make_get_id_from_tag(). Use GitMake::getIdFromTag().');
  return GitMake::getIdFromTag($repo, $tag);
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
// TODO Deprecated. Remove.
function git_make_get_id_from_branch($repo, $branch) {
  drush_log('deprecated: Call to git_make_get_id_from_branch(). Use GitMake::getIdFromBranch().');
  return GitMake::getIdFromBranch($repo, $branch);
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
 *
 * TODO Extend DrushMakeProject class to build out this subtree functionality.
 *  Move this logic in there.
 */
// TODO Move this to GitMake::getSubtree().
function git_make_get_subtree($type, $info, $project) {
  // Determine subtree repo "prefix".
  $prefix = git_make_get_prefix($project['name']);

  // Get path to local copy of the project's git repo. Clone or update if needed.
  $repo = git_make_get_local_repo($project);

  // Get commit ID for project version.
  $commit_id = git_make_get_id($info['core'], $project, $repo);

  // Figure out what branch to use.
  $branch = git_make_get_branch($project);

  // Set commit message.
  $message = sprintf('drush git-make executed: %s', date('Y-m-d, H:i', time()));

  // TODO Add a drush option for --squash TRUE.
  // TODO Why isn't this bringing in full history?
  $squash = FALSE;

  // Top-level directory in git repo.
  $root = getcwd();

  $git_subtree = new GitSubtree($prefix, $repo, $commit_id, $message, $squash,
                                $branch, $root);

  // TODO
  // Recursion(?). Recursion probably shouldn't be an option for git-make.

  if (!is_dir($prefix)) {
    // If the project doesn't exist in the repo yet, add it.
    $command = $git_subtree->add();
    _git_make_exec($command);

    // Garbage collection.
    $command = 'git gc';
    _git_make_exec($command);
  }
  else if (is_dir($prefix)) {
    // TODO Add error handling. E.g. Path can exist but not be a subtree if someone
    //   ran drush make by itself or added the project manually.
    $command = $git_subtree->pull();
    _git_make_exec($command);

    // Garbage collection.
    $command = 'git gc';
    _git_make_exec($command);
  }

  // Set a new commit message, for subtree merge commits.
  $tag = git_make_get_tag($project);
  $message = sprintf('drush git-make executed: %s', date('Y-m-d, H:i', time()));
  $message .= sprintf("\n\ttag: %s", $tag);
  $git_subtree->setMessage($message);
  $command = $git_subtree->merge();
  _git_make_exec($command);
  drush_log("Merged project " . $project['name'] . " (version $tag)");

  // Add project info to info file.
  // @todo Probably don't want to add info to subtree projects. Otherwise this
  //   will get committed and pushed up to d.o, which will be annoying.
  //   Should there be a drush option for people who want to add info here
  //   for some reason?...
  /*
  $revision = ($tag) ? $tag : $commit_id;
  $commands = array();
  $commands[] = git_make_append_project_info($revision, $project['name'], $root, $prefix);
  $commands[] = sprintf('git add %s/%s.info; git commit -m "%s"',
    $prefix, $project['name'], 'Added project info. By drush git-make.'
  );
  foreach ($commands as $command) {
    _git_make_exec($command);
  }
  // */


  // Do Git garbage collection, otherwise this repo can become pretty slow.
  // @todo Add an option to ignore garbage collection.
  // @todo Should we manage garbage collection in ~/.drush-repos/* ?
  $command = 'git gc';
  _git_make_exec($command);

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
// TODO Deprecated. Remove.
function git_make_get_project_path($type, $project) {
  drush_log('deprecated: Call to git_make_get_project_path(). Use GitMake::getProjectPath().');
  return GitMake::getProjectPath($type, $project);
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
// TODO Deprecated. Remove.
function git_make_get_prefix($name) {
  drush_log('deprecated: Call to git_make_get_prefix(). Use GitMake::getPrefix().');
  return GitMake::getPrefix($name);
}

/**
 * @param $project
 *  Array.
 *
 * @return
 *  String. Git branch.
 */
// TODO Deprecated. Remove.
function git_make_get_branch($project) {
  drush_log('deprecated: Call to git_make_get_branch(). Use GitMake::getBranch().');
  return GitMake::getBranch($project);
}

/**
 * Execute all shell commands.
 *
 * @param boolean $ignore_print_only
 *  Ignore 'print-only' command. This should only be used for shell_exec commands
 *  that do NOT change anything. E.g. The git rev-parse command needs to be
 *  executed to get commit IDs and return working commands when print-only runs.
 */
function _git_make_exec($command, $ignore_print_only = FALSE) {
  if (drush_get_option('print-only') && !$ignore_print_only) {
    return drush_print($command);
  }
  else {

    // TODO Add error handling. E.g. detect 'error' and 'fatal' like this...
    /*
    error: pathspec '7.x-.x' did not match any file(s) known to git.
    fatal: Couldn't find remote ref 7.x-.x
    fatal: The remote end hung up unexpectedly
    // */

    // TODO use drush_shell_exec here instead?
    drush_log("Running: $command");
    return shell_exec($command);
  }
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
// TODO Deprecated. Remove.
function git_make_get_version($project) {
  drush_log('deprecated: Call to getVersion(). Use GitMake::getVersion().');
  return GitMake::getVersion($project);
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
// TODO Deprecated. Remove.
function git_make_get_tag($project) {
  drush_log('deprecated: Call to git_make_get_tag(). Use GitMake::getTag().');
  return GitMake::getTag($project);
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
// TODO Deprecated. Remove.
function git_make_checkout_index($type, $info, $project) {
  drush_log('deprecated: Call to git_make_checkout_index(). Use GitMake::checkoutIndex().');
  return GitMake::checkoutIndex($type, $info, $project);
}

/**
 * @param array $project
 *
 * @return boolean
 */
// TODO Deprecated. Remove.
function git_make_download_type_is_subtree($project) {
  return GitMake::downloadTypeIsSubtree($project);
}

/**
 * @return string
 *  Statement to append project info to project info file.
 */
// TODO Deprecated. Remove.
function git_make_append_project_info($revision, $name, $root, $prefix) {
  drush_log('deprecated: Call to git_make_append_project_info(). Use GitMake::appendProjectInfo().');
  return GitMake::appendProjectInfo($revision, $name, $root, $prefix);
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
// TODO Deprecated. Remove.
function git_make_add_symlink($path, $name, $is_core = FALSE) {
  drush_log('deprecated: Call to git_make_add_symlink(). Use GitMake::addSymlink().');
  return GitMake::addSymlink($path, $name, $is_core);
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
// TODO Deprecated. Remove.
function _git_make_add_symlink($path, $name) {
  return GitMake::_addSymlink($path, $name);
}

/**
 * @param string $docroot
 *   Name of root Drupal directory.
 *
 * @return boolean
 *  Success / Failure.
 */
// TODO Deprecated. Remove.
function _git_make_add_docroot($docroot = 'docroot') {
  return GitMake::_addDocroot($docroot);
}

/**
 * Add symlinks to files and directories inside Drupal root.
 *
 * @return string $command
 *  To be executed via shell_exec()
 */
function _git_make_add_core_symlinks() {
  return GitMake::_addCoreSymlinks();
}

/**
 * Remove symlinks to drupal/sites and drupal/profiles.
 *
 * @return string $command
 *  To be executed via shell_exec()
 */
// TODO Deprecated. Remove.
function _git_make_remove_core_symlinks() {
  return GitMake::_removeCoreSymlinks();
}

/**
 * Add docroot/sites/README and docroot/profiles/README.
 *
 * @return string $command
 *  To be executed via shell_exec()
 */
function _git_make_add_core_directories() {
  return GitMake::_addCoreDirectories();
}
