(function ($) {

  Drupal.behaviors.hybridauth_onclick = {};
  Drupal.behaviors.hybridauth_onclick.attach = function(context, settings) {
    $('.hybridauth-onclick-current', context).click(function(index, Element) {
      $(this).parents('.hybridauth-widget').after('<div>' + Drupal.t('Contacting') + ' ' + this.title + '...</div>');
    });
    /*$('.hybridauth-onclick-popup', context).click(function(index, Element) {
      
    });*/
  };

})(jQuery);
