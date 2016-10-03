# JWPlayer 7 Migration Tool

This tool, created for Drupal 7, provides a method of migrating existing jwplayer media objects from SOLR to Youtube using Google API Client Library v2.

### Setup
This module requires the following Drupal 7 modules:

* [jwplayer] - Rest API for jwplayer 7.
* [Composer Manager] - Contrib module for composer package management.

### Installation
This module requires the following packages to run:
* [composer] (https://getcomposer.org/doc/00-intro.md)
* [drush] (http://www.drush.org/en/master/)

Enable the the Drupal 7 modules within the workbench and execute the following.

##### Update the composer packages:
```sh
$ sudo drush composer-json-rebuild -y
$ sudo drush composer-manager install -y
$ sudo drush composer-manager update -y
```

##### Setup Google API Credentials:
* https://console.developers.google.com
* Credentials >> Create credentials >> OAuth client ID
* Select 'Web Application'
* Enter the following in [Authorized redirect URIs]:
    http://localhost/jwplayer_youtube/oauth2callback
* Select 'Create'

##### Setup Drupal Admin Configurations:
* http://localhost:8000/admin/config/media/jwplayer_youtube