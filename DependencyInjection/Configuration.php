<?php

namespace Theapi\CctvBlindfoldBundle\DependencyInjection;

use Symfony\Component\Config\Definition\Builder\TreeBuilder;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * This is the class that validates and merges configuration from your app/config files
 *
 * To learn more see
 * {@link http://symfony.com/doc/current/cookbook/bundles/extension.html#cookbook-bundles-extension-config-class}
 * and
 * {@link http://symfony.com/doc/master/components/config/definition.html}
 */
class Configuration implements ConfigurationInterface
{
    /**
     * {@inheritDoc}
     */
    public function getConfigTreeBuilder()
    {

        $treeBuilder = new TreeBuilder();
        $rootNode = $treeBuilder->root('theapi_cctv_blindfold');

        $rootNode
            ->children()
                ->arrayNode('bluetooth_devices')
                    ->prototype('scalar')->end()
                ->end()
                ->scalarNode('devices')->end()
                ->scalarNode('steps_open')->defaultValue('100')->end()
                ->scalarNode('steps_close')->defaultValue('-100')->end()
             ->end()
        ;

        return $treeBuilder;
    }
}
