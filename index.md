---
layout: page
title: Vaibhav's software engineering musings!
---

# Software engineering musings

This area will contain some of my learnings and coding experiments.

# Post list

<ul class="posts">
  {% for post in site.posts %}
    <li><span>{{ post.date | date_to_string }}</span> &raquo; <a href="{{ BASE_PATH }}{{ post.url }}">{{ post.title }}</a></li>
  {% endfor %}
</ul>

